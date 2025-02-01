#include "compressor-status.h"

volatile CompressorStatus g_compressorStatus = {
    0.0f,  // pressure
    0.0f,  // temperature
    false, // compressorOn
    false, // motorRunning
    false, // airbrushInUse
    0,     // compressionTimerDuration
    0,     // compressionTimeLeft
    0,     // motorTimerDuration
    0,     // motorTimeLeft
    0,     // releaseTimerDuration
    0      // releaseTimeLeft
};
