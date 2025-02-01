#ifndef COMPRESSOR_STATUS_H
#define COMPRESSOR_STATUS_H

#include <stdbool.h>

// Structure to hold all compressor status information
typedef struct {
    float pressure;
    float temperature;
    bool compressorOn;
    bool motorRunning;
    bool airbrushInUse;
    int compressionTimerDuration; // in minutes
    int compressionTimeLeft;      // in minutes
    int motorTimerDuration;       // in minutes
    int motorTimeLeft;            // in minutes
    int releaseTimerDuration;     // in minutes
    int releaseTimeLeft;          // in minutes
} CompressorStatus;

// Declare a global volatile instance.
// Volatile indicates that its value can be updated unexpectedly (by an ISR or another task).
extern volatile CompressorStatus g_compressorStatus;

#endif // COMPRESSOR_STATUS_H
