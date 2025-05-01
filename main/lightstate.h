#ifndef LIGHTSTATE_H
#define LIGHTSTATE_H

#include "esp_event.h"

// Structure for light state
typedef struct {
    bool is_on;          // ON/OFF state
    uint16_t r;          // Red color component (0-4095)
    uint16_t g;          // Green color component (0-4095)
    uint16_t b;          // Blue color component (0-4095)
    uint16_t w;          // White color component (0-4095)
    uint16_t brightness; // Overall brightness (0-4095)
} light_state_t;

#endif // LIGHTSTATE_H