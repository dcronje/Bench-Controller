#include "control.h"
#include "compressor-status.h"
#include "wifi.h"

#include <cstdio>
#include <string>
#include <cstring>

#include "cJSON.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

QueueHandle_t incommingMessageQueue = NULL;

void initControl()
{
  incommingMessageQueue = xQueueCreate(10, sizeof(Message));

  if (!incommingMessageQueue)
  {
    printf("Failed to create incoming message queues.\n");
  }
}

bool bufferToMessage(const char *buffer, Message &msg)
{
  cJSON *json = cJSON_Parse(buffer);
  if (!json)
  {
    printf("Failed to parse JSON: %s\n", buffer);
    return false;
  }

  cJSON *messageType = cJSON_GetObjectItem(json, "messageType");
  if (cJSON_IsString(messageType))
  {
    // If we ever receive a command message (which the control pico ideally shouldn’t),
    // we still parse it.
    if (strcmp(messageType->valuestring, "COMMAND") == 0)
    {
      msg.messageType = COMMAND;
      cJSON *commandType = cJSON_GetObjectItem(json, "commandType");
      if (cJSON_IsString(commandType))
      {
        if (strcmp(commandType->valuestring, "ON") == 0)
          msg.command.commandType = ON;
        else if (strcmp(commandType->valuestring, "OFF") == 0)
          msg.command.commandType = OFF;
        else if (strcmp(commandType->valuestring, "OFF_RELEASE") == 0)
          msg.command.commandType = OFF_RELEASE;
        else if (strcmp(commandType->valuestring, "SET_PRESSURE_TIMEOUT") == 0)
        {
          msg.command.commandType = SET_PRESSURE_TIMEOUT;
          cJSON *timeout = cJSON_GetObjectItem(json, "timeout");
          if (cJSON_IsNumber(timeout))
            msg.command.timeout = timeout->valueint;
        }
        else if (strcmp(commandType->valuestring, "SET_RELEASE_TIMEOUT") == 0)
        {
          msg.command.commandType = SET_RELEASE_TIMEOUT;
          cJSON *timeout = cJSON_GetObjectItem(json, "timeout");
          if (cJSON_IsNumber(timeout))
            msg.command.timeout = timeout->valueint;
        }
        else if (strcmp(commandType->valuestring, "SET_MOTOR_TIMEOUT") == 0)
        {
          msg.command.commandType = SET_MOTOR_TIMEOUT;
          cJSON *timeout = cJSON_GetObjectItem(json, "timeout");
          if (cJSON_IsNumber(timeout))
            msg.command.timeout = timeout->valueint;
        }
        else if (strcmp(commandType->valuestring, "GET_STATUS") == 0)
        {
          msg.command.commandType = GET_STATUS;
        }
        // Add additional command types if needed.
      }
    }
    // Otherwise, if it's an info message from the compressor pico...
    else if (strcmp(messageType->valuestring, "INFO") == 0)
    {
      msg.messageType = INFO;
      cJSON *infoType = cJSON_GetObjectItem(json, "infoType");
      if (cJSON_IsString(infoType))
      {
        // Use a series of if/else to cover the different info types.
        if (strcmp(infoType->valuestring, "STATUS_UPDATE") == 0)
        {
          msg.status.infoType = STATUS_UPDATE;
          cJSON *pressure = cJSON_GetObjectItem(json, "pressure");
          if (cJSON_IsNumber(pressure))
            msg.status.pressure = static_cast<float>(pressure->valuedouble);
          cJSON *temperature = cJSON_GetObjectItem(json, "temperature");
          if (cJSON_IsNumber(temperature))
            msg.status.temperature = static_cast<float>(temperature->valuedouble);
          cJSON *compressorOn = cJSON_GetObjectItem(json, "compressorOn");
          if (cJSON_IsBool(compressorOn))
            msg.status.compressorOn = cJSON_IsTrue(compressorOn);
          cJSON *motorRunning = cJSON_GetObjectItem(json, "motorRunning");
          if (cJSON_IsBool(motorRunning))
            msg.status.motorRunning = cJSON_IsTrue(motorRunning);
          cJSON *airbrushInUse = cJSON_GetObjectItem(json, "airbrushInUse");
          if (cJSON_IsBool(airbrushInUse))
            msg.status.airbrushInUse = cJSON_IsTrue(airbrushInUse);
          cJSON *ctd = cJSON_GetObjectItem(json, "compressionTimerDuration");
          if (cJSON_IsNumber(ctd))
            msg.status.compressionTimerDuration = ctd->valueint;
          cJSON *ctl = cJSON_GetObjectItem(json, "compressionTimeLeft");
          if (cJSON_IsNumber(ctl))
            msg.status.compressionTimeLeft = ctl->valueint;
          cJSON *mtd = cJSON_GetObjectItem(json, "motorTimerDuration");
          if (cJSON_IsNumber(mtd))
            msg.status.motorTimerDuration = mtd->valueint;
          cJSON *mtl = cJSON_GetObjectItem(json, "motorTimeLeft");
          if (cJSON_IsNumber(mtl))
            msg.status.motorTimeLeft = mtl->valueint;
          cJSON *rtd = cJSON_GetObjectItem(json, "releaseTimerDuration");
          if (cJSON_IsNumber(rtd))
            msg.status.releaseTimerDuration = rtd->valueint;
          cJSON *rtl = cJSON_GetObjectItem(json, "releaseTimeLeft");
          if (cJSON_IsNumber(rtl))
            msg.status.releaseTimeLeft = rtl->valueint;
        }
        else if (strcmp(infoType->valuestring, "PRESSURE_CHANGE") == 0)
        {
          msg.status.infoType = PRESSURE_CHANGE;
          cJSON *pressure = cJSON_GetObjectItem(json, "pressure");
          if (cJSON_IsNumber(pressure))
            msg.status.pressure = static_cast<float>(pressure->valuedouble);
        }
        else if (strcmp(infoType->valuestring, "TEMPERATURE_CHANGE") == 0)
        {
          msg.status.infoType = TEMPERATURE_CHANGE;
          cJSON *temperature = cJSON_GetObjectItem(json, "temperature");
          if (cJSON_IsNumber(temperature))
            msg.status.temperature = static_cast<float>(temperature->valuedouble);
        }
        else if (strcmp(infoType->valuestring, "TURNED_ON") == 0)
        {
          msg.status.infoType = TURNED_ON;
        }
        else if (strcmp(infoType->valuestring, "TURNED_OFF") == 0)
        {
          msg.status.infoType = TURNED_OFF;
        }
        else if (strcmp(infoType->valuestring, "RELEASING") == 0)
        {
          msg.status.infoType = RELEASING;
        }
        else if (strcmp(infoType->valuestring, "RELEASED") == 0)
        {
          msg.status.infoType = RELEASED;
        }
        else if (strcmp(infoType->valuestring, "MOTOR_START") == 0)
        {
          msg.status.infoType = MOTOR_START;
        }
        else if (strcmp(infoType->valuestring, "MOTOR_STOP") == 0)
        {
          msg.status.infoType = MOTOR_STOP;
        }
        else if (strcmp(infoType->valuestring, "PRESSURE_COUNTDOWN_UPDATED") == 0)
        {
          msg.status.infoType = PRESSURE_COUNTDOWN_UPDATED;
          // Assuming that countdown updates use the "compressionTimeLeft" field:
          cJSON *timeout = cJSON_GetObjectItem(json, "timeout");
          if (cJSON_IsNumber(timeout))
            msg.status.compressionTimeLeft = timeout->valueint;
        }
        else if (strcmp(infoType->valuestring, "RELEASE_COUNTDOWN_UPDATE") == 0)
        {
          msg.status.infoType = RELEASE_COUNTDOWN_UPDATE;
          cJSON *timeout = cJSON_GetObjectItem(json, "timeout");
          if (cJSON_IsNumber(timeout))
            msg.status.releaseTimeLeft = timeout->valueint;
        }
        else if (strcmp(infoType->valuestring, "MOTOR_COUNTDOWN_UPDATE") == 0)
        {
          msg.status.infoType = MOTOR_COUNTDOWN_UPDATE;
          cJSON *timeout = cJSON_GetObjectItem(json, "timeout");
          if (cJSON_IsNumber(timeout))
            msg.status.motorTimeLeft = timeout->valueint;
        }
        else if (strcmp(infoType->valuestring, "SUPPLY_START") == 0)
        {
          msg.status.infoType = SUPPLY_START;
        }
        else if (strcmp(infoType->valuestring, "SUPPLY_STOP") == 0)
        {
          msg.status.infoType = SUPPLY_STOP;
        }
        else
        {
          printf("Unknown infoType received: %s\n", infoType->valuestring);
          // Optionally, you can return false or assign a default value.
        }
      }
    }
  }
  cJSON_Delete(json);
  return true;
}

std::string messageToString(const Message &msg)
{
  cJSON *json = cJSON_CreateObject();
  if (msg.messageType == COMMAND)
  {
    cJSON_AddStringToObject(json, "messageType", "COMMAND");
    switch (msg.command.commandType)
    {
    case ON:
      cJSON_AddStringToObject(json, "commandType", "ON");
      break;
    case OFF:
      cJSON_AddStringToObject(json, "commandType", "OFF");
      break;
    case OFF_RELEASE:
      cJSON_AddStringToObject(json, "commandType", "OFF_RELEASE");
      break;
    case SET_PRESSURE_TIMEOUT:
      cJSON_AddStringToObject(json, "commandType", "SET_PRESSURE_TIMEOUT");
      cJSON_AddNumberToObject(json, "timeout", msg.command.timeout);
      break;
    case SET_RELEASE_TIMEOUT:
      cJSON_AddStringToObject(json, "commandType", "SET_RELEASE_TIMEOUT");
      cJSON_AddNumberToObject(json, "timeout", msg.command.timeout);
      break;
    case SET_MOTOR_TIMEOUT:
      cJSON_AddStringToObject(json, "commandType", "SET_MOTOR_TIMEOUT");
      cJSON_AddNumberToObject(json, "timeout", msg.command.timeout);
      break;
    case GET_STATUS:
      cJSON_AddStringToObject(json, "commandType", "GET_STATUS");
      break;
    default:
      break;
    }
  }
  else if (msg.messageType == INFO)
  {
    cJSON_AddStringToObject(json, "messageType", "INFO");
    if (msg.status.infoType == STATUS_UPDATE)
    {
      cJSON_AddStringToObject(json, "infoType", "STATUS_UPDATE");
      cJSON_AddNumberToObject(json, "pressure", msg.status.pressure);
      cJSON_AddNumberToObject(json, "temperature", msg.status.temperature);
      cJSON_AddBoolToObject(json, "compressorOn", msg.status.compressorOn);
      cJSON_AddBoolToObject(json, "motorRunning", msg.status.motorRunning);
      cJSON_AddBoolToObject(json, "airbrushInUse", msg.status.airbrushInUse);
      cJSON_AddNumberToObject(json, "compressionTimerDuration", msg.status.compressionTimerDuration);
      cJSON_AddNumberToObject(json, "compressionTimeLeft", msg.status.compressionTimeLeft);
      cJSON_AddNumberToObject(json, "motorTimerDuration", msg.status.motorTimerDuration);
      cJSON_AddNumberToObject(json, "motorTimeLeft", msg.status.motorTimeLeft);
      cJSON_AddNumberToObject(json, "releaseTimerDuration", msg.status.releaseTimerDuration);
      cJSON_AddNumberToObject(json, "releaseTimeLeft", msg.status.releaseTimeLeft);
    }
  }
  char *jsonString = cJSON_PrintUnformatted(json);
  std::string result(jsonString);
  cJSON_free(jsonString);
  cJSON_Delete(json);
  return result;
}

// {"messageType": "INFO", "infoType": "STATUS_UPDATE", "pressure": 101.3, "temperature": 25.6, "compressorOn": true, "motorRunning": false, "airbrushInUse": true, "compressionTimerDuration": 10, "compressionTimeLeft": 1, "motorTimerDuration": 5, "motorTimeLeft": 1, "releaseTimerDuration": 8, "releaseTimeLeft": 1}

void sendOnCommand()
{
  Message msg;
  msg.messageType = COMMAND;
  msg.command.commandType = ON;
  if (xQueueSend(outgoingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
    printf("Failed to send ON command.\n");
  else
    printf("ON command queued.\n");
}

void sendOffCommand()
{
  Message msg;
  msg.messageType = COMMAND;
  msg.command.commandType = OFF;
  if (xQueueSend(outgoingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
    printf("Failed to send OFF command.\n");
  else
    printf("OFF command queued.\n");
}

void sendGetStatusCommand()
{
  Message msg;
  msg.messageType = COMMAND;
  msg.command.commandType = GET_STATUS;
  if (xQueueSend(outgoingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
    printf("Failed to send GET_STATUS command.\n");
  else
    printf("GET_STATUS command queued.\n");
}

void sendSetPressureTimeoutCommand(int minutes)
{
  Message msg;
  msg.messageType = COMMAND;
  msg.command.commandType = SET_PRESSURE_TIMEOUT;
  msg.command.timeout = minutes;
  if (xQueueSend(outgoingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
    printf("Failed to send SET_PRESSURE_TIMEOUT command.\n");
  else
    printf("SET_PRESSURE_TIMEOUT command queued: %d minutes\n", minutes);
}

void sendSetMotorTimeoutCommand(int minutes)
{
  Message msg;
  msg.messageType = COMMAND;
  msg.command.commandType = SET_MOTOR_TIMEOUT;
  msg.command.timeout = minutes;
  if (xQueueSend(outgoingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
    printf("Failed to send SET_MOTOR_TIMEOUT command.\n");
  else
    printf("SET_MOTOR_TIMEOUT command queued: %d minutes\n", minutes);
}

void sendSetSupplyTimeoutCommand(int minutes)
{
  Message msg;
  msg.messageType = COMMAND;
  msg.command.commandType = SET_RELEASE_TIMEOUT;
  msg.command.timeout = minutes;
  if (xQueueSend(outgoingMessageQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS)
    printf("Failed to send SET_RELEASE_TIMEOUT command.\n");
  else
    printf("SET_RELEASE_TIMEOUT command queued: %d minutes\n", minutes);
}

// --- Control Task for Processing Incoming Status Messages ---
// In the control Pico project this task waits on the incoming message queue (populated
// by your socket server) and processes status updates sent from the compressor Pico.

void logCompressor()
{
  float pressure = g_compressorStatus.pressure;
  float temperature = g_compressorStatus.temperature;
  bool compressorOn = g_compressorStatus.compressorOn;
  bool motorRunning = g_compressorStatus.motorRunning;
  bool airbrushInUse = g_compressorStatus.airbrushInUse;
  int compDuration = g_compressorStatus.compressionTimerDuration;
  int compTimeLeft = g_compressorStatus.compressionTimeLeft;
  int motorDuration = g_compressorStatus.motorTimerDuration;
  int motorTimeLeft = g_compressorStatus.motorTimeLeft;
  int releaseDuration = g_compressorStatus.releaseTimerDuration;
  int releaseTimeLeft = g_compressorStatus.releaseTimeLeft;

  // Render the information on your OLED display.
  // For now, we print to the console.
  printf("== Compressor Status ==\n");
  printf("Pressure: %.2f\n", pressure);
  printf("Temperature: %.2f\n", temperature);
  printf("Compressor: %s\n", compressorOn ? "ON" : "OFF");
  printf("Motor: %s\n", motorRunning ? "Running" : "Stopped");
  printf("Airbrush: %s\n", airbrushInUse ? "Active" : "Idle");
  printf("Compression Timer: %d/%d minutes\n", compTimeLeft, compDuration);
  printf("Motor Timer: %d/%d minutes\n", motorTimeLeft, motorDuration);
  printf("Release Timer: %d/%d minutes\n", releaseTimeLeft, releaseDuration);
  printf("=======================\n");
}

void controlTask(void *params)
{
  Message msg;
  while (true)
  {
    if (xQueueReceive(incommingMessageQueue, &msg, portMAX_DELAY) == pdPASS)
    {
      if (msg.messageType == INFO)
      {
        switch (msg.status.infoType)
        {
        case STATUS_UPDATE:
          // Update all fields from the comprehensive status update
          g_compressorStatus.pressure = msg.status.pressure;
          g_compressorStatus.temperature = msg.status.temperature;
          g_compressorStatus.compressorOn = msg.status.compressorOn;
          g_compressorStatus.motorRunning = msg.status.motorRunning;
          g_compressorStatus.airbrushInUse = msg.status.airbrushInUse;
          g_compressorStatus.compressionTimerDuration = msg.status.compressionTimerDuration;
          g_compressorStatus.compressionTimeLeft = msg.status.compressionTimeLeft;
          g_compressorStatus.motorTimerDuration = msg.status.motorTimerDuration;
          g_compressorStatus.motorTimeLeft = msg.status.motorTimeLeft;
          g_compressorStatus.releaseTimerDuration = msg.status.releaseTimerDuration;
          g_compressorStatus.releaseTimeLeft = msg.status.releaseTimeLeft;
          break;
        case PRESSURE_CHANGE:
          // For incremental updates, update only the affected field(s)
          g_compressorStatus.pressure = msg.status.pressure;
          break;
        case TEMPERATURE_CHANGE:
          g_compressorStatus.temperature = msg.status.temperature;
          break;
        case PRESSURE_COUNTDOWN_UPDATED:
          g_compressorStatus.compressionTimeLeft = msg.status.compressionTimeLeft;
          break;
        case RELEASE_COUNTDOWN_UPDATE:
          g_compressorStatus.releaseTimeLeft = msg.status.releaseTimeLeft;
          break;
        case MOTOR_COUNTDOWN_UPDATE:
          g_compressorStatus.motorTimeLeft = msg.status.motorTimeLeft;
          break;
        case TURNED_ON:
          g_compressorStatus.compressorOn = true;
          break;
        case TURNED_OFF:
          g_compressorStatus.compressorOn = false;
          break;
        case MOTOR_START:
          g_compressorStatus.motorRunning = true;
          break;
        case MOTOR_STOP:
          g_compressorStatus.motorRunning = false;
          break;
        case SUPPLY_START:
          g_compressorStatus.airbrushInUse = true;
          break;
        case SUPPLY_STOP:
          g_compressorStatus.airbrushInUse = false;
          break;
        // You can handle additional cases as needed.
        default:
          printf("Unknown info type received: %d\n", msg.status.infoType);
          break;
        }
        logCompressor();
      }
      else
      {
        printf("Received a non-info message (unexpected) on the control Pico.\n");
      }
    }
  }
}
