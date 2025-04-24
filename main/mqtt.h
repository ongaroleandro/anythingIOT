#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "esp_event.h"
#include "mqtt_client.h"

// Structure for light state
typedef struct {
    bool is_on;          // ON/OFF state
    uint16_t r;          // Red color component (0-4095)
    uint16_t g;          // Green color component (0-4095)
    uint16_t b;          // Blue color component (0-4095)
    uint16_t w;          // White color component (0-4095)
    uint16_t brightness; // Overall brightness (0-4095)
} light_state_t;

// Public API function to start MQTT client
void mqtt_app_start(void);

// Function to get current light state (if needed in other modules)
light_state_t* mqtt_get_light_state(void);

#endif // MQTT_APP_H