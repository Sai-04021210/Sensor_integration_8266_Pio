// Receive a string via UART

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "string.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define UART_0_TX 1
#define UART_0_RX 3
#define UART_NUM UART_NUM_0

// WiFi Configuration
#define WIFI_SSID "WLAN-HACKATHON"
#define WIFI_PASS "Viza!Uri25"

// API Configuration
#define API_URL "http://httpbin.org/get"
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char *TAG = "STM32_ESP32";

// WiFi event handler
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retry to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Initialize WiFi
void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init finished.");
}

void init_RS232()
{
    // USB Connection
    const uart_port_t uart_num = UART_NUM;
    const int uart_buffer_size = 1024;
    QueueHandle_t uart_queue;

    // 1 - Setting Communication Parameters
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(uart_num, &uart_config);

    // 2 - Setting Communication Pins
    uart_set_pin(uart_num, UART_0_TX, UART_0_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // 3 - Driver Installation
    uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0);
}

// Queue for HTTP requests
QueueHandle_t http_queue;

// HTTP task
void http_task(void *pvParameters)
{
    char json_data[1024];

    while (1) {
        // Wait for JSON data from UART task
        if (xQueueReceive(http_queue, json_data, portMAX_DELAY)) {
            ESP_LOGI(TAG, "HTTP Task: Sending GET request to %s", API_URL);

            esp_http_client_config_t config = {
                .url = API_URL,
                .method = HTTP_METHOD_GET,
                .timeout_ms = 5000,
            };

            esp_http_client_handle_t client = esp_http_client_init(&config);

            esp_err_t err = esp_http_client_perform(client);

            if (err == ESP_OK) {
                int status_code = esp_http_client_get_status_code(client);
                ESP_LOGI(TAG, "HTTP GET Status = %d", status_code);

                char buffer[256];
                int data_read = esp_http_client_read_response(client, buffer, sizeof(buffer) - 1);
                if (data_read >= 0) {
                    buffer[data_read] = '\0';
                    ESP_LOGI(TAG, "Response: %s", buffer);
                }
            } else {
                ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);
        }
    }
}

static void rx_task()
{
    const uart_port_t uart_num = UART_NUM;
    int length = 0;
    uint8_t rx_data[1024]="\0";
    char json_buffer[1024] = {0};
    int json_pos = 0;
    bool json_started = false;
    int brace_count = 0;

    while (1)
    {
        uart_get_buffered_data_len(uart_num, (size_t *)&length);
        if (length > 0) {
            uart_read_bytes(uart_num, rx_data, length, 100);

            // Display received data
            printf("%.*s", length, rx_data);

            // Process each character to extract JSON
            for (int i = 0; i < length; i++) {
                char c = rx_data[i];

                if (c == '{') {
                    if (!json_started) {
                        json_started = true;
                        json_pos = 0;
                        brace_count = 0;
                    }
                    json_buffer[json_pos++] = c;
                    brace_count++;
                } else if (json_started) {
                    json_buffer[json_pos++] = c;

                    if (c == '{') {
                        brace_count++;
                    } else if (c == '}') {
                        brace_count--;

                        if (brace_count == 0) {
                            // Complete JSON found
                            json_buffer[json_pos] = '\0';
                            ESP_LOGI(TAG, "JSON received: %s", json_buffer);

                            // Send to HTTP task
                            if (xQueueSend(http_queue, json_buffer, 0) != pdTRUE) {
                                ESP_LOGW(TAG, "HTTP queue full, dropping request");
                            }

                            // Reset for next JSON
                            json_started = false;
                            json_pos = 0;
                            memset(json_buffer, 0, sizeof(json_buffer));
                        }
                    }

                    // Prevent buffer overflow
                    if (json_pos >= sizeof(json_buffer) - 1) {
                        json_started = false;
                        json_pos = 0;
                        memset(json_buffer, 0, sizeof(json_buffer));
                    }
                }
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "STM32 to API Bridge - Starting");

    // Initialize WiFi
    wifi_init_sta();

    // Wait for WiFi connection
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Create HTTP queue
    http_queue = xQueueCreate(5, 1024);
    if (http_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create HTTP queue");
        return;
    }

    // Initialize UART
    init_RS232();

    // Start HTTP task with large stack
    xTaskCreate(http_task, "http_task", 1024 * 8, NULL, 5, NULL);

    // Start UART task
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, NULL);

    ESP_LOGI(TAG, "System ready - monitoring STM32 data and sending to API");
}