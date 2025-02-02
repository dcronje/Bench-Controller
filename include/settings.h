#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

// Flash memory constants
#define FLASH_TARGET_OFFSET 0x100000
#define FLASH_SECTOR_SIZE (4 * 1024)
#define SETTINGS_MAGIC 0x1234ABCD

// Structure to store settings
typedef struct
{
  char ssid[32];
  char password[64];
  int authMode;
  int fanSpeed;
  uint32_t magic; // Magic number for validity check
} Settings;

// Commands for the settings queue
typedef enum
{
  SETTINGS_UPDATE, // Update the settings
  SETTINGS_RESET,  // Reset settings to default
} SettingsCommandType;

// Queue and global settings
extern QueueHandle_t settingsQueue;
extern volatile Settings currentSettings;
static Settings localSettingsCopy;

// Function declarations
void initSettings();
void requestSettingsReset();
void requestSettingsUpdate();
void settingsTask(void *params);

#endif // SETTINGS_H
