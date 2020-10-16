#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "nvs_flash.h"          // Handlers of wifi

void scann() {
	wifi_scan_config_t scan_cfg = {
        .ssid = 0,              // Name of AP
        .bssid = 0,             // Mac adress of AP
        .channel = 0,           // Channel with of AP
        .show_hidden = true     // Output hidden networks in terminal 
	};
	printf("Start scanning\n");
	ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, true));
	printf("completed!\n");

	uint8_t ap_num;
	wifi_ap_record_t ap_records[20];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));
    printf("Founded %d acces points\n", ap_num);
    printf("        SSID         |   Channel   |RSSI\n");
    for(int i = 0; i < ap_num; i++) {
        printf("%21s|%13d|%4d\n", ap_records[i].ssid, ap_records[i].primary, ap_records[i].rssi);
    }
}

void app_main() {
    nvs_flash_init();
    tcpip_adapter_init();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);
    esp_wifi_start();
    while(true) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        scann();
    }
}
