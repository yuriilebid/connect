#include "esp_wifi.h"
#include <esp_wifi_types.h>
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"

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

    /* Creating group to check bit mask when we are connected */
    wifi_event_group = xEventGroupCreate();
    
    /* This function should be called exactly once 
     * from application code, when the application starts up. */
    esp_netif_init();

    /* Creates default loops, witch means no handler of this loop for user.
       Used as system loop for WiFi */
    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    // esp_netif_destroy(instance);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // esp_event_handler_instance_t instance_any_id;
    // esp_event_handler_instance_t instance_got_ip;
    /* Register an instance of event handler to the default loop 
       @Note: 
             WIFI_EVENT            - default WiFi mask
             ESP_EVENT_ANY_ID      - its id desirable for a handler */
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &run_on_event, NULL);
    /* @Note:
             IP_EVENT              - default IP mask  
             IP_EVENT_STA_GOT_IP   - mask of IP received */
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &run_on_event, NULL);

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

    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg);

    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,                // Bits masks to wait for
            pdFALSE, pdFALSE, portMAX_DELAY);

    printf("Bits received: %d\n\n", bits);
    if (bits & WIFI_CONNECTED_BIT) {
        printf("connected to ap SSID:%s\n password:%s\n\n", WIFINAME, WIFIPASSWORD);
    }
    else if(bits & WIFI_FAIL_BIT) {
        printf("Failed to connect to ap SSID:%s\n password:%s\n\n", WIFINAME, WIFIPASSWORD);
    }
    else {
    	printf("Unexpected event\n\n");
    }
}









