#include "device_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "esp_mac.h"
#include "esp_random.h"

#define DEVICE_ID_KEY "device_id"
#define LIGHT_STATE_KEY "light_state"
#define DEVICE_ID_LENGTH 6
#define NVS_NAMESPACE "device_cfg"

static const char *TAG = "device_config";
static char device_id[DEVICE_ID_LENGTH + 1]; // +1 for null terminator
static light_state_t current_light_state = {
    .is_on = false,
    .r = 0,
    .g = 0,
    .b = 0,
    .w = 0,
    .brightness = 0
};

// Generate a random alphanumeric string for device ID
void device_config_generate_id(void) {
    //Get the base MAC address from different sources
    uint8_t base_mac_addr[6] = {0};
    esp_err_t ret = ESP_OK;

    //Get base MAC address from EFUSE BLK0(default option)
    ret = esp_read_mac(base_mac_addr, ESP_MAC_EFUSE_FACTORY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get base MAC address from EFUSE BLK0. (%s)", esp_err_to_name(ret));

        // generate it with a random seed then
        const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";

        // Seed the random number generator
        srand(esp_random());

        // Generate random ID
        for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
            int index = rand() % (sizeof(charset) - 1);
            device_id[i] = charset[index];
        }
        device_id[DEVICE_ID_LENGTH] = '\0';  // Null terminate the string
        
    } else {
        ESP_LOGI(TAG, "Base MAC Address read from EFUSE BLK0: %s", base_mac_addr);
        
        //Set the base MAC address using the retrieved MAC address
        ESP_LOGI(TAG, "Using \"0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\" as base MAC address",
            base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);
        esp_iface_mac_addr_set(base_mac_addr, ESP_MAC_BASE);

        // Use the last 3 bytes of MAC address to create a 6 character device_id
        snprintf(device_id, 7, "%02X%02X%02X", base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);
        ESP_LOGI(TAG, "Generated new device ID: %s", device_id);
    }
    
}

bool device_config_init(void) {
    esp_err_t err;
    
    // Initialize NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return false;
    }
    
    // Try to load the device ID
    size_t required_size = sizeof(device_id);
    err = nvs_get_str(nvs_handle, DEVICE_ID_KEY, device_id, &required_size);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Generate and store a new device ID
        device_config_generate_id();
        err = nvs_set_str(nvs_handle, DEVICE_ID_KEY, device_id);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error storing device ID: %s", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return false;
        }
        
        // Commit changes to NVS
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return false;
        }
        
        ESP_LOGI(TAG, "New device ID created and stored: %s", device_id);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading device ID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    } else {
        ESP_LOGI(TAG, "Loaded device ID: %s", device_id);
    }
    
    // Try to load the light state
    size_t light_state_size = sizeof(light_state_t);
    err = nvs_get_blob(nvs_handle, LIGHT_STATE_KEY, &current_light_state, &light_state_size);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved light state found, using defaults");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading light state: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Loaded light state - On: %d, R: %d, G: %d, B: %d, W: %d, Brightness: %d",
                current_light_state.is_on, current_light_state.r, current_light_state.g,
                current_light_state.b, current_light_state.w, current_light_state.brightness);
    }
    
    nvs_close(nvs_handle);
    return true;
}

char* device_config_get_id(void) {
    return device_id;
}

light_state_t* device_config_get_light_state(void) {
    return &current_light_state;
}

bool device_config_store_light_state(light_state_t* state) {
    // Update our local copy
    if (state != &current_light_state) {  // Don't copy if it's the same pointer
        memcpy(&current_light_state, state, sizeof(light_state_t));
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return false;
    }
    
    err = nvs_set_blob(nvs_handle, LIGHT_STATE_KEY, &current_light_state, sizeof(light_state_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error storing light state: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS data: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    
    ESP_LOGI(TAG, "Stored light state - On: %d, R: %d, G: %d, B: %d, W: %d, Brightness: %d",
            current_light_state.is_on, current_light_state.r, current_light_state.g,
            current_light_state.b, current_light_state.w, current_light_state.brightness);
    
    nvs_close(nvs_handle);
    return true;
}