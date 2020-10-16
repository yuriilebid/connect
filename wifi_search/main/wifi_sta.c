#include "esp_wifi.h"
#include <esp_wifi_types.h>
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "nvs_flash.h"

#define WIFINAME "WIFI"
#define WIFIPASSWORD "lebid1234"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t wifi_event_group;

/* Function connected to an event handler */
void run_on_event(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    // Event handler logic
}

void app_main() {
    // Initialisation of NVS (non volatile storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("Log - 1\n");
    /* Creating group to check bit mask when we are connected */
    wifi_event_group = xEventGroupCreate();
    printf("Log - 2\n");
    /* This function should be called exactly once 
     * from application code, when the application starts up. */
    ESP_ERROR_CHECK(esp_netif_init());
    printf("Log - 3\n");
    /* Creates default loops, witch means no handler of this loop for user.
       Used as system loop for WiFi */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    printf("Log - 4\n");
    esp_netif_create_default_wifi_sta();
    printf("Log - 5\n");
    // esp_netif_destroy(instance);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    printf("Log - 6\n");
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    printf("Log - 7\n");
    esp_event_handler_t instance_any_id;
    esp_event_handler_t instance_got_ip;
    /* Register an instance of event handler to the default loop 
       @Note: 
             WIFI_EVENT            - default WiFi mask
             ESP_EVENT_ANY_ID      - its id desirable for a handler */
    printf("Log - 8\n");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &run_on_event, NULL));
    /* @Note:
             IP_EVENT              - default IP mask  
             IP_EVENT_STA_GOT_IP   - mask of IP received */
    printf("Log - 9\n");
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &run_on_event, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFINAME,
            .password = WIFIPASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,          // Setting minimum requirements of securty in list of wifi acces points
            // .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,          // Sort match AP in scan list by RSSI (Received Signal Strength Indicator)
            .pmf_cfg = {                                       // Protected Management Frames config
            	.capable = true,                               // Supports pmf connection if device has it
            	.required = false                              // Works with devices without pmf
            },
        },
    };

    printf("Log - 10\n");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    printf("Log - 11\n");
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));

    printf("Log - 12\n");
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("Log - 13\n");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, 
            pdFALSE, 
            portMAX_DELAY);
    printf("Log - 14\n");
    // printf("Bits received: %d\n\n", bits);
    if (bits & WIFI_CONNECTED_BIT) {
        printf("connected to ap SSID:%s\n password:%s\n\n", WIFINAME, WIFIPASSWORD);
    }
    else if(bits & WIFI_FAIL_BIT) {
        printf("Failed to connect to ap SSID:%s\n password:%s\n\n", WIFINAME, WIFIPASSWORD);
    }
    else {
    	printf("Unexpected event\n\n");
    }
    // esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    // esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    vEventGroupDelete(wifi_event_group);
}









