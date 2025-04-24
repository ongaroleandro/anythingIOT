#include <stdio.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "esp_system.h"
#include "esp_log.h"

#include "wifi_manager.h"

#include "mqtt.h"

#include "esp_ws28xx.h"
#define LED_GPIO 6
#define LED_NUM 30

CRGB* ws2812_buffer;

static const char TAG_wifi[] = "Wi-Fi";
void cb_connection_ok(void *pvParameter){
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG_wifi, "I have a connection and my IP is %s!", str_ip);

	mqtt_app_start();
}

// MQTT topics (based on the Python code)
//static const char *discovery_prefix = "homeassistant";
//static char device_id[7];

static const char TAG_led[] = "LED";
static void configure_led(void)
{
    ESP_LOGI(TAG_led, "Initialised LED strip");
    ws28xx_init(LED_GPIO, WS2812B, LED_NUM, &ws2812_buffer);
}

static void set_led(void)
{
    light_state_t* stLightState = mqtt_get_light_state();
    for (int i = 0; i < LED_NUM; i++) {
        if (!stLightState->is_on)
            ws2812_buffer[i] = (CRGB){.r = 0, .g = 0, .b = 0};
        else
            ws2812_buffer[i] = (CRGB){.r = stLightState->r, .g = stLightState->g, .b = stLightState->b};
    }
    ws28xx_update();
}

void led_control(void *pvParameters) {
    /* Configure the peripheral according to the LED type */
    configure_led();
    while (1) {
        set_led();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    /* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

	// Create a FreeRTOS task
    ESP_LOGI(TAG_led, "Started led_control");
    xTaskCreate(&led_control, "led_control", 2048, NULL, 5, NULL);
}