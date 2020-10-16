#include "esp_wifi.h"
#include <esp_wifi_types.h>
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define WIFINAME "WIFI"
#define WIFIPASSWORD "lebid1234"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

/* Function connected to an event handler */
static esp_err_t run_on_event(void *ctx, system_event_t *event) {
    switch(event->event_id) {
		
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    
	case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    
	default:
        break;
    }
   
	return ESP_OK;
}

void app_main() {
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);
    // Initialisation of NVS (non volatile storage)
    ESP_ERROR_CHECK(nvs_flash_init());

    printf("Log - 0\n");
    /* Creating group to check bit mask when we are connected */
    wifi_event_group = xEventGroupCreate();
    // initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(run_on_event, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFINAME,
            .password = WIFIPASSWORD,
            // .threshold.authmode = WIFI_AUTH_WPA2_PSK,          // Setting minimum requirements of securty in list of wifi acces points
            // // .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,          // Sort match AP in scan list by RSSI (Received Signal Strength Indicator)
            // .pmf_cfg = {                                       // Protected Management Frames config
            // 	.capable = true,                               // Supports pmf connection if device has it
            // 	.required = false                              // Works with devices without pmf
            // },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    // printf("Bits received: %d\n\n", bits);
    printf("Main task: waiting for connection to the wifi network... ");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    printf("connected!\n");
    // print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	while(true) {
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}









