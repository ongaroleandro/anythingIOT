#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include <cJSON.h>
#include "mqtt.h"
#include "device_config.h"


static const char *TAG_mqtt = "mqtt";

// MQTT topics - will be set dynamically based on device ID
static char config_topic[64];
static char command_topic[64];
static char state_topic[64];
static char unique_id[64];

// Global light state
static light_state_t stLightState = {
    .is_on = false,
    .r = 0,
    .g = 0,
    .b = 0,
    .w = 0,
    .brightness = 0
};

// Forward declarations
static void publish_config(esp_mqtt_client_handle_t client);
static void publish_init_state(esp_mqtt_client_handle_t client);
static bool parse_mqtt_message(const char *payload, light_state_t *state);
static char *create_config(void);
static void setup_topics(void);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_mqtt, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_mqtt, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, command_topic, 0);
        publish_config(client);

        publish_init_state(client);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        if (parse_mqtt_message(event->data, &stLightState)) {
            // Store the new state in NVS
            device_config_store_light_state(&stLightState);

            esp_mqtt_client_publish(client, state_topic, event->data, event->data_len, 0, true);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_mqtt, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG_mqtt, "Other event id:%d", event->event_id);
        break;
    }
}

// Create and publish initial state JSON
static void publish_init_state(esp_mqtt_client_handle_t client) {
    light_state_t* last_known_state = device_config_get_light_state();
    stLightState.is_on = last_known_state->is_on;
    stLightState.g = last_known_state->g;
    stLightState.b = last_known_state->b;
    stLightState.w = last_known_state->w;
    stLightState.brightness = last_known_state->brightness;
    stLightState.r = last_known_state->r;

    cJSON *root = cJSON_CreateObject();
    
    // Add state (ON/OFF)
    cJSON_AddStringToObject(root, "state", stLightState.is_on ? "ON" : "OFF");
    
    // Add brightness
    cJSON_AddNumberToObject(root, "brightness", stLightState.brightness);
    
    // Add color
    cJSON *color = cJSON_CreateObject();
    cJSON_AddNumberToObject(color, "r", stLightState.r);
    cJSON_AddNumberToObject(color, "g", stLightState.g);
    cJSON_AddNumberToObject(color, "b", stLightState.b);
    cJSON_AddNumberToObject(color, "w", stLightState.w);

    // Add color JSON to state json
    cJSON_AddItemToObject(root, "color", color);
    
    // Convert to string
    char *payload = cJSON_Print(root);
    
    // Publish
    esp_mqtt_client_publish(client, state_topic, payload, 0, 0, true);
    ESP_LOGI(TAG_mqtt, "Published state: %s", payload);
    
    // Cleanup
    free(payload);
    cJSON_Delete(root);
}

// Set up topic strings based on device ID
static void setup_topics(void) {
    char *device_id = device_config_get_id();
    
    snprintf(config_topic, sizeof(config_topic), "homeassistant/light/%s_light/config", device_id);
    snprintf(command_topic, sizeof(command_topic), "homeassistant/light/%s_light/set", device_id);
    snprintf(state_topic, sizeof(state_topic), "homeassistant/light/%s_light/state", device_id);
    snprintf(unique_id, sizeof(unique_id), "%s_light", device_id);
    
    ESP_LOGI(TAG_mqtt, "Topics configured with device ID %s", device_id);
    ESP_LOGI(TAG_mqtt, "Config topic: %s", config_topic);
    ESP_LOGI(TAG_mqtt, "Command topic: %s", command_topic);
    ESP_LOGI(TAG_mqtt, "State topic: %s", state_topic);
}

char *create_config(void)
{
	char *string = NULL;
	cJSON *supported_color_modes = NULL;
	cJSON *supported_color_modes_string = NULL;
	
	cJSON *identifier = NULL;
	cJSON *identifier_string = NULL;
	
	cJSON *config = cJSON_CreateObject();
	
	if (cJSON_AddStringToObject(config, "name", "REGEBELEEGHT") == NULL)
	{
		goto end;
	}
	
	cJSON_AddStringToObject(config, "command_topic", command_topic);
	cJSON_AddStringToObject(config, "state_topic", state_topic);
	cJSON_AddStringToObject(config, "unique_id", unique_id);
	cJSON_AddStringToObject(config, "platform", "mqtt");
	
	//create device JSON
	cJSON *device = cJSON_CreateObject();
	identifier = cJSON_AddArrayToObject(device, "ids");
    char *id_string = device_config_get_id();
	identifier_string = cJSON_CreateString(id_string); //could also use CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count); if more than 1 color
	cJSON_AddItemToArray(identifier, identifier_string);
	cJSON_AddStringToObject(device, "name", "OngaroLight");
	cJSON_AddStringToObject(device, "mf", "Ongaro");
	cJSON_AddStringToObject(device, "mdl", "blingbling");
	cJSON_AddStringToObject(device, "sw", "alpha");
	cJSON_AddNumberToObject(device, "sn", 124589);
	
	// add device JSON to the config JSON
	cJSON_AddItemToObject(config, "device", device);
	
	cJSON_AddStringToObject(config, "schema", "json");
	cJSON_AddTrueToObject(config, "brightness");
	cJSON_AddNumberToObject(config, "brightness_scale", 4095);
	supported_color_modes = cJSON_AddArrayToObject(config, "supported_color_modes");
	supported_color_modes_string = cJSON_CreateString("rgbw"); //could also use CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count); if more than 1 color
	cJSON_AddItemToArray(supported_color_modes, supported_color_modes_string);	
	string = cJSON_Print(config);
	printf("%s \n", string);
	
	
end:
	cJSON_Delete(config);
	return string;
}

bool parse_mqtt_message(const char *payload, light_state_t *state) {
    cJSON *root = cJSON_Parse(payload);
    if (root == NULL) {
        return false;
    }

    // Always check and set state
    cJSON *state_json = cJSON_GetObjectItemCaseSensitive(root, "state");
    if (cJSON_IsString(state_json) && (state_json->valuestring != NULL)) {
        state->is_on = (strcmp(state_json->valuestring, "ON") == 0);
    }

    // Color parsing - only update if color is present
    cJSON *color_json = cJSON_GetObjectItemCaseSensitive(root, "color");
    if (cJSON_IsObject(color_json)) {
        cJSON *r = cJSON_GetObjectItemCaseSensitive(color_json, "r");
        cJSON *g = cJSON_GetObjectItemCaseSensitive(color_json, "g");
        cJSON *b = cJSON_GetObjectItemCaseSensitive(color_json, "b");
        cJSON *w = cJSON_GetObjectItemCaseSensitive(color_json, "w");

        state->r = cJSON_IsNumber(r) ? r->valueint : state->r;
        state->g = cJSON_IsNumber(g) ? g->valueint : state->g;
        state->b = cJSON_IsNumber(b) ? b->valueint : state->b;
        state->w = cJSON_IsNumber(w) ? w->valueint : state->w;
    }

    // Brightness parsing
    cJSON *brightness_json = cJSON_GetObjectItemCaseSensitive(root, "brightness");
    state->brightness = cJSON_IsNumber(brightness_json) ? 
                        brightness_json->valueint : state->brightness;

    cJSON_Delete(root);
    return true;
}

// Function to publish configuration topics
static void publish_config(esp_mqtt_client_handle_t client) {
    char *my_config = create_config();
    esp_mqtt_client_publish(client, config_topic, my_config, 0, 1, true);
    ESP_LOGI(TAG_mqtt, "Published configuration topics");
    free(my_config);
}

// Public function to access light state from other modules
light_state_t* mqtt_get_light_state(void) {
    return &stLightState;
}

void mqtt_app_start(void)
{
    // Set up topics using the device ID
    setup_topics();
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .credentials.username = CONFIG_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_MQTT_PASSWORD, 
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}