#ifndef CONTROL_H
#define CONTROL_H

#include <string>
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"
#include "cJSON.h"

// Shared message definitions and enums

typedef enum
{
  COMMAND,
  INFO
} MessageType;

typedef enum
{
  ON,
  OFF,
  OFF_RELEASE,
  SET_COMPRESSION_TIMEOUT,
  SET_RELEASE_TIMEOUT,
  SET_MOTOR_TIMEOUT,
  GET_STATUS
} CommandType;

typedef enum
{
  TURNED_ON,
  TURNED_OFF,
  RELEASING,
  RELEASED,
  PRESSURE_CHANGE,
  MOTOR_START,
  MOTOR_STOP,
  PRESSURE_COUNTOWN_END,
  RELEASE_COUNTDOWN_END,
  MOTOR_COUNTDOWN_END,
  PRESSURE_COUNTDOWN_UPDATED,
  RELEASE_COUNTDOWN_UPDATE,
  MOTOR_COUNTDOWN_UPDATE,
  SUPPLY_START,
  SUPPLY_STOP,
  TEMPERATURE_CHANGE,
  STATUS_UPDATE
} InfoType;

// Message structure using a union for command vs. status fields.
typedef struct
{
  MessageType messageType;
  union
  {
    struct
    { // For commands (sent from the control Pico)
      CommandType commandType;
      int timeout; // Used for timeout commands
    } command;
    struct
    { // For status messages (received from the compressor Pico)
      InfoType infoType;
      float pressure;
      float temperature;
      int timeout; // Used for countdown updates, if needed

      // Comprehensive status fields:
      bool compressorOn;
      bool motorRunning;
      bool airbrushInUse;
      int compressionTimerDuration; // Total duration set for compressor operation
      int compressionTimeLeft;      // Minutes remaining before shutdown
      int motorTimerDuration;
      int motorTimeLeft;
      int releaseTimerDuration;
      int releaseTimeLeft;
    } status;
  };
} Message;

// Queue handles shared with the socket module
extern QueueHandle_t incommingMessageQueue;

void initControl();

// Command helper functions for the control Pico
void sendOnCommand();
void sendOffCommand();
void sendGetStatusCommand();
void sendSetCompressionTimeoutCommand(int minutes);
void sendSetMotorTimeoutCommand(int minutes);
void sendSetReleaseTimeoutCommand(int minutes);

// The control task processes incoming messages (status updates) from the compressor Pico.
// (This replaces the hardware control task used on the compressor Pico.)
void controlTask(void *params);

// Functions for converting messages to/from JSON
bool bufferToMessage(const char *buffer, Message &msg);
std::string messageToString(const Message &msg);

#endif // CONTROL_H
