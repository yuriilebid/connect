#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/ledc.h"
#include <regex.h> 
#include "esp_types.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/i2c.h"
#include <unistd.h>
#include "esp_err.h"
#include "driver/i2s.h"
#include <strings.h>
#include <ctype.h>
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <netdb.h>

#define DHT11_POWER 2
#define DHT11_DATA  4
// #include <cjson/json.h>

QueueHandle_t queue = NULL;

bool command_line_status = false;

#define UART_PORT UART_NUM_2
#define PORT 80

#define TXPIN 16
#define RXPIN 17

#define WIFI_STORAGE			"WIFI_data"

#define CMD_MAX_LEN 30
#define LF_ASCII_CODE 13
