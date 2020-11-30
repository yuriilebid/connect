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
    while(connection_status != 1) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    	vTaskDelay(1000);
    }
    if(connection_status == 1) {
        save_wifi_data_to_nvs(name, password);
    }
}

static void inline nvs_init() {
	while (nvs_flash_init() == ESP_ERR_NVS_NO_FREE_PAGES) {
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


void get_send(int sock) {
    char buff[2048];

    bzero(buff, 2048);
    sprintf(buff, "GET / HTTP/1.0\n\r\n\r");
    send(sock, buff, strlen(buff), 0);
}


void get_receive(int sock) {
    char buff[20480];

    bzero(buff, 20480);
    recv(sock, buff, 20480, 0);
    printf("%s\n", buff);
}

void http_request(char* adr) {
    char full_url[strlen(adr) + 10];
    char path[strlen(adr)];
    int back_slash_ind = 0;

    strcpy(path, strchr(adr, '/'));
    if(path == NULL) {
        printf("Ok buddy\n\n");
    }
    bzero(full_url, strlen(adr) + 10);
    for(int i = 0; i < strlen(adr) && adr[i - 1] != '/'; i++)
        if(adr[i] == '/') adr[i] = '\0';
    // strcpy(full_url, "http://");
    strcat(full_url, adr);
    printf("full url: %s\n", full_url);
    printf("%s\n", path);
    // socket_fd = socket_connect();

    struct hostent *server_host = gethostbyname("google.com");
    printf("Big error 1\n");
    // printf("%d\n", server_host->h_addrtype);
    char* host = inet_ntoa(*(struct in_addr*)server_host->h_addr);
    printf("Big error 2\n");
    printf("%s\n", host);

    int socket_fd;
    struct sockaddr_in sock_in;

    sock_in.sin_port = htons(PORT);
    sock_in.sin_addr.s_addr = inet_addr(host);
    sock_in.sin_family = AF_INET;
    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    printf("Big error 3\n");
    int status = connect(socket_fd, (struct sockaddr_in*)&sock_in, sizeof(sock_in));
    
    printf("status = %d\n", status);
    printf("Big error 4\n");

    get_send(socket_fd);
    printf("Big error 5\n");
    get_receive(socket_fd);
    printf("Big error 6\n");
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
            connect_ssid(arr[1], arr[2]);
        }
        else if(strstr(cmd, "http_get") == cmd) {
            printf("cmd: %s\n", cmd);
            char** arr = get_parameters(cmd);

            http_request(arr[1]);
        }
        command_line_status = false;
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
                    uart_write_bytes(UART_NUM_2, "\r\n\r", strlen("\r\n\r"));
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
    nvs_init();
    queue = xQueueCreate(1, CMD_MAX_LEN);
    
    xTaskCreate(uart_conlose_input, "input_getter", 12040u, NULL, 5, 0);
    xTaskCreate(handle_cmd, "handle_cmd", 12040u, NULL, 5, 0);
    wifi_init();
    wifi_auto_connect();
}
