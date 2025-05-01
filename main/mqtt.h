#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "esp_event.h"
#include "mqtt_client.h"
#include "lightstate.h"

// Public API function to start MQTT client
void mqtt_app_start(void);

// Function to get current light state (if needed in other modules)
light_state_t* mqtt_get_light_state(void);

#endif // MQTT_APP_H