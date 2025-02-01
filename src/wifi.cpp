#include "wifi.h"
#include "constants.h"
#include "dhcpserver.h"
#include "httpserver.h"
#include "settings.h"
#include "control.h"
#include "ws2812.pio.h"

#include <cstdio>
#include <string>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "hardware/pio.h"

#include "lwip/apps/httpd.h"
#include "lwip/apps/mdns.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwipopts.h"

#include "pico/cyw43_arch.h"

#define STARTUP_BIT (1 << 0)
#define WIFI_CONNECTED_BIT (1 << 1)
#define WIFI_CONNECTION_FAILED_BIT (1 << 2)
#define CONFIGURED_BIT (1 << 3)
#define SOCKET_SERVER_FAILED_BIT (1 << 4)

EventGroupHandle_t eventGroup;

WifiScanResult topScanResults[MAX_SCAN_RESULTS];
int scanResultCount = 0;
dhcp_server_t dhcpServer;

volatile bool isTestingConnection = false;
volatile bool isConnectedToWifi = false;
volatile bool cyw43IsInitialised = false;
volatile bool isAppModeActive = false;
volatile bool isStaModeActive = false;
volatile bool isSocketActive = false;
volatile bool isConnectedToSocketServer = false;

QueueHandle_t outgoingMessageQueue = NULL;
static int serverSocket = -1;
volatile static int socketRetryDelay = 5000;
volatile static int wifiRetryDelay = 1000;
volatile bool isFlashing = false;

volatile bool mdnsAp = false;
volatile bool mdnsSta = false;

PIO pio;
uint sm;

static s8_t mdnsServiceHandleSta = -1;
static s8_t mdnsServiceHandleAp = -1;

void sendSS2812Color(PIO pio, uint sm, uint32_t rgb_color)
{
  uint32_t encoded_color = ((rgb_color >> 8 & 0xFF) << 16) | ((rgb_color >> 16 & 0xFF) << 8) | (rgb_color & 0xFF); // GRB format
  pio_sm_put_blocking(pio, sm, encoded_color << 8 | 0x7);                                                          // 24 bits + 1 extra to meet reset condition
}

void printSettings(const volatile Settings *settings)
{
  printf("SSID: %s\nPassword: %s\nAuth Mode: %d\nMagic Number: %d\n",
         settings->ssid,
         settings->password,
         settings->authMode,
         settings->magic);
}

// Minimal TXT callback that does nothing.
void srv_txt(struct mdns_service *service, void *txt_userdata)
{
  (void)service;
  (void)txt_userdata;
  // No TXT records to add.
}

void deinitMdnsSta()
{
  if (!mdnsSta)
  {
    return;
  }
  struct netif *sta_netif = &cyw43_state.netif[CYW43_ITF_STA];
  cyw43_arch_lwip_begin();
  // If you have a valid service handle, explicitly delete the service:
  if (mdnsServiceHandleSta >= 0)
  {
    mdns_resp_del_service(sta_netif, mdnsServiceHandleSta);
    mdnsServiceHandleSta = -1;
  }
  // Now remove the netif from the mDNS responder.
  mdns_resp_remove_netif(sta_netif);
  printf("mDNS on STA netif removed.\n");
  cyw43_arch_lwip_end();
  mdnsSta = false;
}

void initMdnsSta()
{
  if (mdnsSta)
  {
    return;
  }
  // Get a pointer to the STA network interface.
  struct netif *sta_netif = &cyw43_state.netif[CYW43_ITF_STA];
  if (!sta_netif)
  {
    printf("STA netif is null.\n");
    return;
  }

  // Set the hostname using the provided API.
  netif_set_hostname(sta_netif, MDNS_DOMAIN);

  // Begin lwIP critical section.
  cyw43_arch_lwip_begin();

  // Add the network interface with the hostname.
  mdns_resp_add_netif(sta_netif, netif_get_hostname(sta_netif));

  // Advertise a service.
  // Here, we advertise a service instance "ControlPico" of type "_control" on TCP port 12345.
  mdnsServiceHandleSta = mdns_resp_add_service(sta_netif, MDNS_NAME, MDNS_SERVICE, DNSSD_PROTO_TCP, 12345, srv_txt, NULL);
  if (mdnsServiceHandleSta != 0)
  {
    printf("mDNS add service failed for STA: %d\n", mdnsServiceHandleSta);
    cyw43_arch_lwip_end();
    return;
  }

  // Announce the service.
  mdns_resp_announce(sta_netif);
  printf("mDNS for STA initialized and service advertised.\n");

  // End lwIP critical section.
  cyw43_arch_lwip_end();
  mdnsSta = true;
}

void deinitMdnsAp()
{
  if (!mdnsAp)
  {
    return;
  }
  struct netif *sta_netif = &cyw43_state.netif[CYW43_ITF_AP];
  cyw43_arch_lwip_begin();
  // If you have a valid service handle, explicitly delete the service:
  if (mdnsServiceHandleAp >= 0)
  {
    mdns_resp_del_service(sta_netif, mdnsServiceHandleAp);
    mdnsServiceHandleAp = -1;
  }
  // Now remove the netif from the mDNS responder.
  mdns_resp_remove_netif(sta_netif);
  printf("mDNS on AP netif removed.\n");
  cyw43_arch_lwip_end();
  mdnsAp = false;
}

// Helper function to initialize and advertise mDNS for AP mode.
void initMdnsAp()
{
  if (mdnsAp)
  {
    return;
  }
  // Get a pointer to the AP network interface.
  struct netif *ap_netif = &cyw43_state.netif[CYW43_ITF_AP];
  if (!ap_netif)
  {
    printf("AP netif is null.\n");
    return;
  }

  // Set the hostname using the provided API.
  netif_set_hostname(ap_netif, MDNS_DOMAIN);

  // Begin lwIP critical section.
  cyw43_arch_lwip_begin();

  // Add the network interface with the hostname.
  mdns_resp_add_netif(ap_netif, netif_get_hostname(ap_netif));

  // Advertise a service.
  // Here, we advertise a service instance "ControlPico" of type "_control" on TCP port 12345.
  s8_t err = mdns_resp_add_service(ap_netif, MDNS_NAME, MDNS_SERVICE, DNSSD_PROTO_TCP, 12345, srv_txt, NULL);
  if (err != 0)
  {
    printf("mDNS add service failed for AP: %d\n", err);
    cyw43_arch_lwip_end();
    return;
  }

  // Announce the service.
  mdns_resp_announce(ap_netif);
  printf("mDNS for AP initialized and service advertised.\n");

  // End lwIP critical section.
  cyw43_arch_lwip_end();
  mdnsAp = true;
}

// Sort scan results by RSSI in descending order
void sortScanResultsByRSSI()
{
  for (int i = 0; i < scanResultCount - 1; i++)
  {
    for (int j = 0; j < scanResultCount - i - 1; j++)
    {
      if (topScanResults[j].rssi < topScanResults[j + 1].rssi)
      {
        WifiScanResult temp = topScanResults[j];
        topScanResults[j] = topScanResults[j + 1];
        topScanResults[j + 1] = temp;
      }
    }
  }
}

// Supported security types (filter others)
bool isSupportedSecurity(int auth_mode)
{
  // Allow networks that the Pico can actually connect to
  return auth_mode == 5 || // WPA with TKIP
         auth_mode == 7;   // WPA2 with AES (or mixed)
}

// Add or replace SSID in the top results (sorted by RSSI)
void addToTopResults(const char *ssid, int rssi, int auth_mode)
{
  // Ignore empty SSIDs
  if (strlen(ssid) == 0)
  {
    return;
  }

  // Check if the SSID already exists in the list
  for (int i = 0; i < scanResultCount; i++)
  {
    if (strcmp(topScanResults[i].ssid, ssid) == 0)
    {
      // Update RSSI and auth_mode if the new RSSI is stronger
      if (rssi > topScanResults[i].rssi)
      {
        topScanResults[i].rssi = rssi;
        topScanResults[i].auth_mode = auth_mode;
      }
      return;
    }
  }

  // If we haven't filled the list, add the SSID
  if (scanResultCount < MAX_SCAN_RESULTS)
  {
    strncpy(topScanResults[scanResultCount].ssid, ssid, SSID_MAX_LEN);
    topScanResults[scanResultCount].ssid[SSID_MAX_LEN] = '\0'; // Null-terminate
    topScanResults[scanResultCount].rssi = rssi;
    topScanResults[scanResultCount].auth_mode = auth_mode;
    scanResultCount++;
  }
  else
  {
    // Replace the weakest entry if the new one is stronger
    int weakestIndex = 0;
    for (int i = 1; i < MAX_SCAN_RESULTS; i++)
    {
      if (topScanResults[i].rssi < topScanResults[weakestIndex].rssi)
      {
        weakestIndex = i;
      }
    }

    if (rssi > topScanResults[weakestIndex].rssi)
    {
      strncpy(topScanResults[weakestIndex].ssid, ssid, SSID_MAX_LEN);
      topScanResults[weakestIndex].ssid[SSID_MAX_LEN] = '\0';
      topScanResults[weakestIndex].rssi = rssi;
      topScanResults[weakestIndex].auth_mode = auth_mode;
    }
  }
}

int wifiScanCallback(void *env, const cyw43_ev_scan_result_t *result)
{
  if (result)
  {
    // Convert SSID and check supported security
    if (isSupportedSecurity(result->auth_mode))
    {
      addToTopResults(reinterpret_cast<const char *>(result->ssid), result->rssi, result->auth_mode);
    }
  }
  else
  {
    printf("Scan complete\n");
  }
  return 0;
}

// Function to perform Wi-Fi scan
bool performWifiScan()
{
  printf("Starting Wi-Fi scan...\n");
  scanResultCount = 0; // Reset the results buffer

  cyw43_wifi_scan_options_t scanOptions = {0};
  int result = cyw43_wifi_scan(&cyw43_state, &scanOptions, NULL, wifiScanCallback);

  if (result != 0)
  {
    printf("Wi-Fi scan failed with error code: %d\n", result);
    return false;
  }

  // Wait for the scan to complete
  while (cyw43_wifi_scan_active(&cyw43_state))
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  sortScanResultsByRSSI();

  return true;
}

// Function to print scan results (for debugging)
void printScanResults()
{
  printf("Wi-Fi scan results:\n");
  if (scanResultCount == 0)
  {
    printf("No results found.\n");
    return;
  }
  for (int i = 0; i < scanResultCount; i++)
  {
    printf("SSID: %s, RSSI: %d, Auth Mode: %d\n", topScanResults[i].ssid, topScanResults[i].rssi, topScanResults[i].auth_mode);
  }
}

int mapAuthMode(int input)
{
  switch (input)
  {
  case 0:
    return CYW43_AUTH_OPEN;
  case 1:
    return CYW43_AUTH_WPA_TKIP_PSK;
  case 7:
    return CYW43_AUTH_WPA2_AES_PSK; // Observed correspondence
  case 3:
    return CYW43_AUTH_WPA2_MIXED_PSK;
  case 4:
    return CYW43_AUTH_WPA3_SAE_AES_PSK;
  case 5:
    return CYW43_AUTH_WPA3_WPA2_AES_PSK;
  default:
    printf("Unknown auth mode: %d. Defaulting to open.\n", input);
    return CYW43_AUTH_OPEN;
  }
}

void checkWifiConnection()
{
  if (!cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA))
  {
    printf("WiFi connection dropped.\n");
    // Attempt reconnection or other handling logic
    isConnectedToWifi = false;
  }
  else
  {
    isConnectedToWifi = true;
  }
}

void initArch()
{
  if (!cyw43IsInitialised)
  {
    if (cyw43_arch_init())
    {
      printf("Failed to initialize Wi-Fi module.\n");
      return;
    }
    printf("Wi-Fi module initialized successfully.\n");
    cyw43IsInitialised = true;
  }
}

bool connectToWiFi(const char *ssid, const char *password, int auth_mode)
{
  printf("Using credentials SSID: %s Password: %s Auth Mode: %d\n", ssid, password, auth_mode);

  if (!isTestingConnection && !isConnectedToWifi)
  {
    isTestingConnection = true;
    const int timeoutMs = 30000; // Increased timeout
    const int retryDelayMs = 2000;

    for (int attempt = 1; attempt <= WIFI_MAX_RETRY; ++attempt)
    {
      printf("Attempting to connect to SSID: %s (Attempt %d/%d)\n", ssid, attempt, WIFI_MAX_RETRY);

      int result = cyw43_arch_wifi_connect_timeout_ms(ssid, password, mapAuthMode(auth_mode), timeoutMs);

      if (result == 0)
      {
        printf("Successfully connected to Wi-Fi.\n");
        isConnectedToWifi = true;
        isTestingConnection = false;
        auto ip_addr = cyw43_state.netif[CYW43_ITF_STA].ip_addr.addr;
        printf("Pico W IP Address: %lu.%lu.%lu.%lu\n", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24);
        initMdnsSta();
        return true;
      }
      else
      {
        printf("Failed to connect to Wi-Fi (Error %d). Retrying...\n", result);
        cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
        vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
      }
    }

    printf("Failed to connect to Wi-Fi after %d attempts.\n", WIFI_MAX_RETRY);
    isTestingConnection = false;
  }
  return false;
}

void disconnectWiFi()
{
  if (isConnectedToWifi)
  {
    deinitMdnsSta();
    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    printf("DisConnectedToWifi from Wi-Fi.\n");
    isConnectedToWifi = false;
  }
}

bool hasCredentials()
{
  if (strlen((const char *)currentSettings.ssid) > 0 && strlen((const char *)currentSettings.password) > 0)
  {
    return true;
  }
  return false;
}

void disconnectAndForgetWifi()
{
  requestSettingsReset();
  vTaskDelay(pdMS_TO_TICKS(500));
  disconnectWiFi();
}

bool connectToWifiWithCredentials()
{
  printSettings(&currentSettings);
  return connectToWiFi((const char *)currentSettings.ssid, (const char *)currentSettings.password, currentSettings.authMode);
}

bool sendMessage(const std::string &message)
{
  if (outgoingMessageQueue == NULL)
  {
    printf("Message queue is not initialized.\n");
    return false;
  }

  if (xQueueSend(outgoingMessageQueue, &message, pdMS_TO_TICKS(100)) == pdPASS)
  {
    printf("Message queued: %s\n", message.c_str());
    return true;
  }
  else
  {
    printf("Failed to queue message: %s\n", message.c_str());
    return false;
  }
}

void initSocket()
{
  if (!isSocketActive)
  {
    Message msg;
    // flush queues
    while (xQueueReceive(incommingMessageQueue, &msg, 0) == pdTRUE)
      ;
    while (xQueueReceive(outgoingMessageQueue, &msg, 0) == pdTRUE)
      ;
    xTaskCreate(serverSocketTask, "ServerSocketTask", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    isSocketActive = true;
  }
}

void deInitSocket()
{
  if (isSocketActive)
  {
    Message msg;
    // flush queues
    while (xQueueReceive(incommingMessageQueue, &msg, 0) == pdTRUE)
      ;
    while (xQueueReceive(outgoingMessageQueue, &msg, 0) == pdTRUE)
      ;
    isSocketActive = false;
  }
}

void initAPMode()
{
  if (!isAppModeActive)
  {
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    printf("AP mode enabled.\n");

    // Initialize DHCP server
    ip_addr_t ip, nm;
    IP4_ADDR(ip_2_ip4(&ip), 192, 168, 4, 1);   // Set AP's IP address
    IP4_ADDR(ip_2_ip4(&nm), 255, 255, 255, 0); // Set subnet mask
    dhcp_server_init(&dhcpServer, &ip, &nm);

    auto ip_addr = cyw43_state.netif[CYW43_ITF_AP].ip_addr.addr;
    printf("Pico W IP Address: %lu.%lu.%lu.%lu\n", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24);

    // Start HTTP server for Wi-Fi configuration
    startHttpServer();
    initMdnsAp();
    isAppModeActive = true;
  }
}

void deInitAPMode()
{
  if (isAppModeActive)
  {
    // Stop HTTP server and DHCP server
    deinitMdnsAp();
    stopHttpServer();
    dhcp_server_deinit(&dhcpServer);
    cyw43_arch_disable_ap_mode();
    printf("AP mode dis-abled.\n");
    isAppModeActive = false;
  }
}

void initSTAMode()
{
  if (!isStaModeActive)
  {
    cyw43_arch_enable_sta_mode();
    isStaModeActive = true;
  }
}

void deInitSTAMode()
{
  if (isStaModeActive)
  {
    deinitMdnsSta();
    cyw43_arch_disable_sta_mode();
    isStaModeActive = false;
  }
}

void credentialsTask(void *params)
{
  while (true)
  {
    if (hasCredentials())
    {
      xEventGroupSetBits(eventGroup, CONFIGURED_BIT);
      vTaskDelete(NULL);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void serverSocketTask(void *params)
{
  // Create the server socket.
  serverSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0)
  {
    printf("Server: Failed to create socket.\n");
    xEventGroupSetBits(eventGroup, SOCKET_SERVER_FAILED_BIT);
    vTaskDelete(NULL);
    return;
  }

  // Set up the server address structure.
  struct sockaddr_in serverAddr;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SOCKET_SERVER_PORT);
  serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces

  // Bind the socket.
  if (lwip_bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
  {
    printf("Server: Bind failed.\n");
    lwip_close(serverSocket);
    serverSocket = -1;
    xEventGroupSetBits(eventGroup, SOCKET_SERVER_FAILED_BIT);
    vTaskDelete(NULL);
    return;
  }
  printf("Server: Bound to port %d\n", SOCKET_SERVER_PORT);

  // Listen for incoming connections.
  if (lwip_listen(serverSocket, 1) < 0)
  {
    printf("Server: Listen failed.\n");
    lwip_close(serverSocket);
    serverSocket = -1;
    xEventGroupSetBits(eventGroup, SOCKET_SERVER_FAILED_BIT);
    vTaskDelete(NULL);
    return;
  }
  printf("Server: Listening for connections...\n");

  while (true)
  {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = lwip_accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket < 0)
    {
      printf("Server: Accept failed.\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    printf("Server: Client connected.\n");

    // Set the client socket to non-blocking mode.
    int flags = lwip_fcntl(clientSocket, F_GETFL, 0);
    if (flags < 0)
    {
      printf("Server: Failed to get socket flags.\n");
      lwip_close(clientSocket);
      continue;
    }
    if (lwip_fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      printf("Server: Failed to set socket non-blocking.\n");
      lwip_close(clientSocket);
      continue;
    }

    bool clientConnected = true;

    sendGetStatusCommand();

    std::string messageBuffer;

    while (clientConnected)
    {
      char tempBuffer[256]; // Temporary buffer for each read.
      int bytesRead = lwip_recv(clientSocket, tempBuffer, sizeof(tempBuffer) - 1, 0);
      if (bytesRead > 0)
      {
        tempBuffer[bytesRead] = '\0';
        // Append the received chunk to the message buffer.
        messageBuffer.append(tempBuffer);

        // Process complete messages delimited by newline.
        size_t pos = 0;
        while ((pos = messageBuffer.find('\n')) != std::string::npos)
        {
          std::string jsonMessage = messageBuffer.substr(0, pos);
          messageBuffer.erase(0, pos + 1);
          printf("Server received complete message: %s\n", jsonMessage.c_str());

          Message msg;
          if (bufferToMessage(jsonMessage.c_str(), msg))
          {
            if (xQueueSend(incommingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
            {
              printf("Server: Failed to enqueue incoming message.\n");
            }
          }
          else
          {
            printf("Server: Failed to convert buffer to Message.\n");
          }
        }
      }
      else if (bytesRead == 0)
      {
        // The connection has been gracefully closed by the client.
        printf("Server: Client disconnected (zero bytes received).\n");
        clientConnected = false;
        break;
      }
      else // bytesRead < 0
      {
        int err_val = errno;
        // If errno is EWOULDBLOCK, EAGAIN, or even 0 (as a workaround), consider it as "no data available" and continue.
        if (err_val == EWOULDBLOCK || err_val == EAGAIN || err_val == 0)
        {
          // No data available; do nothing and continue.
        }
        else
        {
          printf("Server: lwip_recv error: %d. Closing connection.\n", err_val);
          clientConnected = false;
          break;
        }
      }

      // Process outgoing messages regardless of incoming data.
      Message outgoingMsg;
      while (xQueueReceive(outgoingMessageQueue, &outgoingMsg, 0) == pdPASS)
      {
        std::string jsonString = messageToString(outgoingMsg);
        if (lwip_send(clientSocket, jsonString.c_str(), jsonString.length(), 0) < 0)
        {
          printf("Server: Failed to send message: %s\n", jsonString.c_str());
          clientConnected = false;
          break;
        }
        else
        {
          printf("Server: Sent message: %s\n", jsonString.c_str());
        }
      }

      // Yield to allow other tasks to run.
      vTaskDelay(pdMS_TO_TICKS(100));
    }

    lwip_close(clientSocket);
    printf("Server: Connection closed. Waiting for new connection...\n");
  }

  // Cleanup: (never reached)
  lwip_close(serverSocket);
  serverSocket = -1;
  xEventGroupSetBits(eventGroup, SOCKET_SERVER_FAILED_BIT);
  vTaskDelete(NULL);
}

void handleStartup()
{
  initSTAMode();
  if (hasCredentials() && connectToWifiWithCredentials())
  {
    printf("Connected to Wi-Fi!\n");
    xEventGroupSetBits(eventGroup, WIFI_CONNECTED_BIT);
  }
  else
  {
    // No valid credentials, switch to AP mode
    printf("No valid Wi-Fi credentials found. Switching to AP mode...\n");
    performWifiScan();
    printScanResults();
    xEventGroupSetBits(eventGroup, WIFI_CONNECTION_FAILED_BIT);
  }
}

void handleWifiFailed()
{
  deInitSTAMode();
  initAPMode();
  xTaskCreate(credentialsTask, "CredentialsTask", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void handleConfigured()
{
  deInitAPMode();
  initSTAMode();
  if (connectToWifiWithCredentials())
  {
    xEventGroupSetBits(eventGroup, WIFI_CONNECTED_BIT);
  }
  else
  {
    requestSettingsReset();
    vTaskDelay(pdMS_TO_TICKS(500));
    xEventGroupSetBits(eventGroup, WIFI_CONNECTION_FAILED_BIT);
  }
}

void handleWifiConnected()
{
  initSocket();
}

void handleSocketServerFailed()
{
  printf("Deinit socket\n");
  deInitSocket();
  printf("Checking Wifi\n");
  checkWifiConnection();
  if (!isConnectedToWifi)
  {
    printf("Wifi disconnected\n");
    xEventGroupSetBits(eventGroup, STARTUP_BIT);
  }
  else
  {
    printf("Waiting\n");
    vTaskDelay(pdMS_TO_TICKS(socketRetryDelay));
    if (socketRetryDelay < 120000)
    {
      socketRetryDelay += 5000;
    }
    printf("Re initialising\n");
    xEventGroupSetBits(eventGroup, WIFI_CONNECTED_BIT);
  }
}

void updateLedColor()
{
  if (isConnectedToSocketServer)
  {
    sendSS2812Color(pio, sm, 0x00FF00); // Green
  }
  else if (isConnectedToWifi)
  {
    sendSS2812Color(pio, sm, 0xFFFF00); // Orange
  }
  else if (isTestingConnection)
  {
    sendSS2812Color(pio, sm, isFlashing ? 0x000000 : 0xFFFF00); // Flashing Orange
    isFlashing = !isFlashing;
  }
  else if (isAppModeActive)
  {
    sendSS2812Color(pio, sm, 0x0000FF); // Blue
  }
  else
  {
    sendSS2812Color(pio, sm, 0xFFFFFF); // White
  }
}

void initWifi()
{
  pio = pio0;

  // Load the WS2812 program into the PIO memory
  uint offset = pio_add_program(pio, &ws2812_program);

  // Configure the PIO state machine
  pio_sm_config c = ws2812_program_get_default_config(offset);
  sm_config_set_out_pins(&c, WS2812_GPIO, 1);  // Set output pin
  sm_config_set_sideset_pins(&c, WS2812_GPIO); // Set side-set pin (optional, only if used)
  sm_config_set_clkdiv(&c, 2.0f);              // Set clock divider to adjust data rate

  sm = pio_claim_unused_sm(pio, true); // Claim a state machine
  pio_sm_init(pio, sm, offset, &c);    // Initialize the state machine with configuration
  pio_sm_set_enabled(pio, sm, true);   // Enable the state machine

  eventGroup = xEventGroupCreate();
  incommingMessageQueue = xQueueCreate(5, sizeof(Message));
  if (incommingMessageQueue == NULL)
  {
    printf("Failed to create incoming message queue.\n");
  }
  outgoingMessageQueue = xQueueCreate(5, sizeof(Message));
  if (outgoingMessageQueue == NULL)
  {
    printf("Failed to create outgoing message queue.\n");
  }
}

void wifiTask(void *params)
{
  printf("Wi-Fi Task started.\n");
  initArch();
  cyw43_arch_lwip_begin();
  mdns_resp_init();
  cyw43_arch_lwip_end();
  xEventGroupSetBits(eventGroup, STARTUP_BIT);
  while (true)
  {
    EventBits_t bits = xEventGroupWaitBits(
        eventGroup,
        STARTUP_BIT | WIFI_CONNECTED_BIT | WIFI_CONNECTION_FAILED_BIT | CONFIGURED_BIT | SOCKET_SERVER_FAILED_BIT,
        pdTRUE,         // Clear on exit
        pdFALSE,        // Wait for any bit
        portMAX_DELAY); // Wait indefinitely

    if (bits & STARTUP_BIT)
    {
      printf("HANDLE STARTUP\n");
      handleStartup();
    }

    if (bits & WIFI_CONNECTION_FAILED_BIT)
    {
      printf("HANDLE WIFI FAILED\n");
      handleWifiFailed();
    }

    if (bits & CONFIGURED_BIT)
    {
      printf("HANDLE CONFIGURED\n");
      handleConfigured();
    }

    if (bits & WIFI_CONNECTED_BIT)
    {
      printf("HANDLE WIFI CONNECTED\n");
      handleWifiConnected();
    }

    if (bits & SOCKET_SERVER_FAILED_BIT)
    {
      printf("HANDLE SOCKET DISCONNECTED\n");
      handleSocketServerFailed();
    }
  }
}

void ledTask(void *params)
{
  while (1)
  {
    updateLedColor();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}