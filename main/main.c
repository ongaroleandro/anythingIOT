#include <stdio.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "esp_system.h"
#include "esp_log.h"

#include "wifi_manager.h"

#include "mqtt.h"

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

void app_main(void)
{
    /* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
}