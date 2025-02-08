#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

#include "pico/stdlib.h"

void sharedISR(uint gpio, uint32_t events);

#endif // ISR_HANDLERS_H
