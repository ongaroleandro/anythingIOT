#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "mqtt.h"  // For light_state_t structure

// Initialize device configuration
// Returns true if initialization was successful
bool device_config_init(void);

// Get the device ID string
// The returned pointer is valid until the next call to this function
char* device_config_get_id(void);

// Get the stored light state
// Returns a pointer to the internally stored light state
light_state_t* device_config_get_light_state(void);

// Store the current light state to NVS
// Returns true if storage was successful
bool device_config_store_light_state(light_state_t* state);

// Generate a new 6-character device ID
void device_config_generate_id(void);

#endif // DEVICE_CONFIG_H