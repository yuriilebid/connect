#include "header.h"

int connection_status = 0;


char *mx_strnew(int size) {
    char *memory = malloc(size + 1);

    if (memory == NULL) {
        char *msg = "malloc returned NULL";
        write(2, msg, strlen(msg));
        exit(1);
    }
    for (int i = 0; i < size + 1; i++)
        memory[i] = '\0';
    return memory;
}


char *mx_string_copy(char *str) {
    char *copy = mx_strnew(strlen(str));

    for(int i = 0; str[i]; i++)
        copy[i] = str[i];
    return copy;
}


static void inline save_wifi_data_to_nvs(char *name, char *password) {
	nvs_handle_t my_handle;

    nvs_open(WIFI_STORAGE, NVS_READWRITE, &my_handle);
    nvs_set_str(my_handle, name, password);
    nvs_close(my_handle);
}

static void connect_ssid(char *name, char *password) {
    wifi_config_t wifi_sta_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,          // Setting minimum requirements of securty in list of wifi acces points
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,          // Sort match AP in scan list by RSSI (Received Signal Strength Indicator)
            .pmf_cfg = {                                       // Protected Management Frames config
                .capable = true,                               // Supports pmf connection if device has it
                .required = false                              // Works with devices without pmf
            },
        },
    };
    memcpy(wifi_sta_cfg.sta.ssid,     name, strlen(name));
    memcpy(wifi_sta_cfg.sta.password, password, strlen(password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_cfg);

    esp_wifi_disconnect();
    vTaskDelay(100); 
    char *str_load = "\n\rTrying to connect N ";
    char *str_conf = "\n\r\e[0;32m> Connected to WiFi!\e[0;39m\n\r";
    char *dis_conf = "\n\r\e[0;31m> Error during connection!\e[0;39m\n\r";
    char buff[50];

    for(int i = 0; connection_status != 1 && i < 10; i++) {
        esp_wifi_connect();
        save_wifi_data_to_nvs(name, password);
        bzero(buff, 50);
        sprintf(&buff, "%s%d\n\r", str_load, i);
        uart_write_bytes(UART_NUM_2, buff, strlen(buff));
        vTaskDelay(500);
    }
    uart_write_bytes(UART_NUM_2, str_conf, strlen(str_conf));
    vTaskDelay(500);
}

static void inline nvs_init() {
	if (nvs_flash_init() == ESP_ERR_NVS_NO_FREE_PAGES) {
		nvs_flash_erase();
		nvs_flash_init();
	}
}

void event_handler(void *arg, esp_event_base_t event_base, int event_id, void *event_data) {
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        connection_status = 0;
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    	connection_status = 1;
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
    	connection_status = 0;
    }
    vTaskDelay(10);
}


/*
 * @Function : 
 *             uart_init
 *
 * @Description : 
 *                Configure UART interface. UART used for console input
*/

static void inline uart_init() {
    const int uart_buffer_size = (1024 * 2);

	uart_config_t uart_cfg = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	uart_param_config(UART_PORT, &uart_cfg);
    uart_set_pin(UART_NUM_2, RXPIN, TXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, uart_buffer_size, 0, 0, NULL, 0);
}

static void inline wifi_init() {
	esp_netif_init();
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // esp_wifi_start();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);
    esp_wifi_start();
}


void wifi_auto_connect() {
   esp_err_t err;
    esp_wifi_start();
    esp_wifi_scan_start(NULL, true);

    uint16_t ap_num;
    wifi_ap_record_t ap_records[20];
    bzero(ap_records, 20);
    esp_wifi_scan_get_ap_records(&ap_num, ap_records);

    nvs_handle_t my_handle;
    nvs_open(WIFI_STORAGE, NVS_READWRITE, &my_handle);
    char ap_ssid[100];
    char *password;
    size_t required_size;

    for(int i = 0; i < ap_num; i++) {
        if ((char *)ap_records[i].ssid == NULL) {continue;}
        bzero(ap_ssid, 100);
        sprintf(ap_ssid, "%s", ap_records[i].ssid);

        required_size = 0;
        nvs_get_str(my_handle, ap_ssid, NULL, &required_size);
        if ((int)required_size == 0) {
            continue;
        }

        password = mx_strnew(required_size);
        nvs_get_str(my_handle, ap_ssid, password, &required_size);
        if (strlen(password) != 0) {
            connect_ssid(ap_ssid, password);
            printf("Error 1\n");
            free(password);
        }
        free(password);
    }
    nvs_close(my_handle);
}


char **mx_strarr_new(int size) {
    int len = size + 1;
    char **data = (char **)malloc(len * sizeof(char *));
    if (data == NULL) exit(1);
    for (int i = 0; i < len; ++i) data[i] = NULL;
    return data;
}


static char** get_parameters(char* cmd_in) {
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


void echo_ping(char *ip, int port, int times) {
    int net_socket;

    net_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_adr;

    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons(port);
    server_adr.sin_addr.s_addr = inet_addr(ip);

    int connection_status = connect(net_socket, (struct sockaddr *) &server_adr, sizeof(server_adr));

    if(connection_status != 0) {
        printf("%d\n", errno);
        printf("Connection failed!\n");
        // exit(1);
    }
    else {
        printf("Connection done!\n");
    }
    if(connection_status != 0) {
        xQueueSendToBack(error, "\e[1m\e[0;31mConnection failed\e[0;39m", 0);
        // status = false;
    }

    for(int i = 0; i < times + 1; i++) {
        char server_response[128];

        bzero(server_response, 128);
        recv(net_socket, &server_response, sizeof(server_response), 0);
        uart_write_bytes(UART_NUM_2, "\n\r", 2);
        uart_write_bytes(UART_NUM_2, (char*)server_response, strlen((char*)server_response));
        uart_write_bytes(UART_NUM_2, "\n\r", 2);
        
    }
    close(net_socket);
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
            char** arr = get_parameters(cmd);

            // printf("name: %s\npassword: %s\n\n", name, pass);
            if(arr[3] != NULL) {
                char ssid_name[strlen(arr[1]) + strlen(arr[2]) + 2];
                
                bzero(ssid_name, strlen(arr[1]) + strlen(arr[2]) + 2);
                sprintf(&ssid_name, "%s %s", arr[1], arr[2]);
                connect_ssid(ssid_name, arr[3]);
            }
            else {
                connect_ssid(arr[1], arr[2]);
            }
        }
        else if(strstr(cmd, "echo") == cmd)  {
            printf("cmd: %s\n", cmd);
            char** arr = get_parameters(cmd);

            echo_ping(arr[1], atoi(arr[2]), atoi(arr[3]));
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


/*
 * @Function : 
 *             uart_conlose_input
 *
 * @Parameters : 
 *               arg        - NULL (needs to be a vTask)
 *
 * @Description : Gets input from UART console with managin function buttons.
 *                Sends input to queue "queue".
*/

static void uart_conlose_input(void* arg) {
    uint8_t buff[CMD_MAX_LEN];
    uint8_t err[ERR_MAX_LEN];
    char* text_pick = "> ";

    uart_write_bytes(UART_NUM_2, "\n\r", 2);
    while(true) {
        char *request = NULL;
        bool end_cmd = false;

        bzero(buff, CMD_MAX_LEN);
        bzero(err, ERR_MAX_LEN);
        uart_write_bytes(UART_NUM_2, text_pick, strlen(text_pick));
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
        xQueueSendToBack(queue, &buff, 0);
        if(xQueueReceive(error, &err, 5) != errQUEUE_EMPTY) {
            uart_write_bytes(UART_NUM_2, "\n\r", 2);
            uart_write_bytes(UART_NUM_2, (char*)err, strlen((char*)err));

            text_pick = "\e[0;31m> \e[0;39m";
        }
        else {
            text_pick = "\e[0;32m> \e[0;39m";
        }
        uart_write_bytes(UART_NUM_2, "\n\r", 2);
    }
}



void app_main() {
    uart_init();
    nvs_init();
    wifi_init();
    wifi_auto_connect();
    queue = xQueueCreate(1, CMD_MAX_LEN);
    error = xQueueCreate(1, ERR_MAX_LEN);
    printf("Error 2\n");
    
    xTaskCreate(uart_conlose_input, "input_getter", 32768u, NULL, 5, 0);
    xTaskCreate(handle_cmd, "handle_cmd", 16384u, NULL, 5, 0);
    printf("Error 3\n");
}
