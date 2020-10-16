#include "esp_wifi.h"
#include <esp_wifi_types.h>
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define WIFINAME "WIFI"
#define WIFIPASSWORD "lebid1234"

#define CMD_MAX_LEN 30
#define LF_ASCII_CODE 13

#define TXPIN 16
#define RXPIN 17

#define STORAGE_NAMESPACE "APs_storage"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

bool command_line_status = false;

QueueHandle_t queue = NULL;


/*
 * @Function : 
 *             uart_init
 *
 * @Description : 
 *                Configure UART interface. UART used for console input
*/

void uart_init() {
    const int uart_buffer_size = (1024 * 2);

    uart_config_t uart_cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_NUM_2, &uart_cfg);
    uart_set_pin(UART_NUM_2, RXPIN, TXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, uart_buffer_size, 0, 0, NULL, 0);
}


/* Read from NVS APs history list
   Return an error if anything goes wrong
   during this process.
 */

char* check_ap(char* ssid) {
    nvs_handle_t my_handle;
    esp_err_t err;
    char* password;

    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    err = nvs_get_str(my_handle, ssid, &password, NULL);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        password = NULL;
    }
    nvs_close(my_handle);
    return password;
}


/* Save the number of module restarts in NVS
   by first reading and then incrementing
   the number that has been saved previously.
   Return an error if anything goes wrong
   during this process.
 */

esp_err_t save_ssid(char* ssid, char* password) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        return err;
    }
    // Write
    err = nvs_set_str(my_handle, ssid, password);
    if (err != ESP_OK) {
        return err;
    }
    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        return err;
    }
    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

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

void connect_ssid(char* name, char* pass) {
    printf("name: %s\n", name);
    printf("pass: %s\n", pass);
    wifi_config_t wifi_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,          // Setting minimum requirements of securty in list of wifi acces points
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,          // Sort match AP in scan list by RSSI (Received Signal Strength Indicator)
            .pmf_cfg = {                                       // Protected Management Frames config
                .capable = true,                               // Supports pmf connection if device has it
                .required = false                              // Works with devices without pmf
            },
        },
    };
    memcpy(wifi_cfg.sta.ssid, name, strlen(name));
    memcpy(wifi_cfg.sta.password, pass, strlen(pass));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Main task: waiting for connection to the wifi network... ");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    printf("connected!\n");

    tcpip_adapter_ip_info_t ip_info;
    wifi_ap_record_t ap_info;

    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    printf("\nSuccess! Got IP:  %s\n\n", ip4addr_ntoa(&ip_info.ip));
    printf("sta_state\nState: ");
    printf("CONNECTED\n");
    printf("SSID: %s\n", ap_info.ssid);
    printf("Channel: %d\n", ap_info.primary);
    printf("RRSI: %d dBm\n", ap_info.rssi);
}


char **mx_strarr_new(int size) {
    int len = size + 1;
    char **data = (char **)malloc(len * sizeof(char *));
    if (data == NULL) exit(1);
    for (int i = 0; i < len; ++i) data[i] = NULL;
    return data;
}


char** get_param(char* cmd_in) {
    char **cmd = mx_strarr_new(100);

    int index = 0;
    char *p;
    p = strtok(cmd_in, " ");
    cmd[index] = p;
    index++;
    while(p != NULL && index < 100) {
        p = strtok(NULL, " ");
        cmd[index] = p;
        index++;
    }
    int cmd_len = 0;
    while(cmd[cmd_len] && cmd_len < 100) cmd_len++;
    return cmd;
}


/*
 * @Function : 
 *             handle_cmd
 *
 * @Parameters : 
 *               pvParameters        - NULL (needs to be a vTask)
 *
 * @Description : 
 *                Manage witch function to call according to
 *                console input.
*/

void handle_cmd(void *pvParameters) {
    while(true) {
        char cmd[CMD_MAX_LEN];
        bzero(&cmd, CMD_MAX_LEN);
        xQueueReceive(queue, &cmd, CMD_MAX_LEN);

        if(strstr(cmd, "connect") == cmd)  {
            printf("cmd: %s\n", cmd);
            char** arr = get_param(cmd);

            // printf("name: %s\npassword: %s\n\n", name, pass);
            connect_ssid(arr[1], arr[2]);
        }
        command_line_status = false;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


/*
 * @Function : 
 *             input_getter
 *
 * @Parameters : 
 *               pvParameters        - NULL (needs to be a vTask)
 *
 * @Description : Gets input from UART console with managin function buttons.
 *                Sends input to queue "queue".
*/

void input_getter(void *pvParameters) {
    uint8_t buff[CMD_MAX_LEN];

    while(true) {
        char *request = NULL;
        bool end_cmd = false;

        bzero(buff, CMD_MAX_LEN);
        for(int ind = 0; ind < (CMD_MAX_LEN - 1) && !end_cmd;) {
            uart_flush_input(UART_NUM_2);
            int exit = uart_read_bytes(UART_NUM_2, &buff[ind], 1, (200 / portTICK_PERIOD_MS));
            if(exit == 1) {
                char *tmp = (char *)&buff[ind];
                if(buff[ind] == LF_ASCII_CODE) {
                    end_cmd = true;
                    buff[ind] = '\0';
                }
                else if(buff[ind] == 127 && ind == 0) {
                    buff[ind] = '\0';
                }
                else if(buff[ind] == 127) {
                    uart_write_bytes(UART_NUM_2, "\033[D \033[D\033[D", strlen("\033[D \033[D"));
                    buff[ind] = '\0';
                    buff[ind - 1] = '\0';
                    ind--;
                }
                else {
                    uart_write_bytes(UART_NUM_2, (const char*)tmp, 1);
                    ind++;
                }
                vTaskDelay(5 / portTICK_PERIOD_MS);
            }
        }
        command_line_status = true;
        xQueueSendToBack(queue, &buff, 0);
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    uart_init();
    queue = xQueueCreate(1, CMD_MAX_LEN);
    /* Initialisation of NVS (non volatile storage) */
    ESP_ERROR_CHECK(nvs_flash_init());
    /* Creating new event group to check bit mask when we are connected */
    wifi_event_group = xEventGroupCreate();
    /* initialize the tcp stack */
	tcpip_adapter_init();
	/* Create the event handler and task */
	ESP_ERROR_CHECK(esp_event_loop_init(run_on_event, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // xTaskCreate(connect_ssid, "connect_ssid", 2048u, NULL, 5, 0);
    xTaskCreate(input_getter, "input_getter", 12040u, NULL, 5, 0);
    xTaskCreate(handle_cmd, "handle_cmd", 12040u, NULL, 5, 0);
}









