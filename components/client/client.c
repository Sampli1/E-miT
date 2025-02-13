#include <stdio.h>


#include "client.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "spiffs_handler.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))




static const char *TAG = "HTTP_CLIENT";

esp_http_client_handle_t client_http;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            
            // Clean buffer
            if (output_len == 0 && evt->user_data) {
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }


            int copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
            if (evt->user_data && copy_len > 0) {
                memcpy((char*) evt->user_data + output_len, evt->data, copy_len);
            } else if (!evt->user_data) {
                // dynamic buffer
                if (output_buffer == NULL) {
                    output_buffer = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                    output_len = 0;
                }
                if (copy_len > 0) {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }

            output_len += copy_len; 

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                #if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                #endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}


int get_api(char *content, const char* api_address, esp_http_client_handle_t client, char **header_keys, char **header_values, int header_keys_length) {
    int content_length = 0;
    esp_http_client_set_url(client, api_address);
    esp_http_client_set_method(client, HTTP_METHOD_GET);


    ESP_LOGI(TAG, "URL: %s", api_address);

    if (header_keys != NULL) {
        esp_http_client_delete_header(client, "Content-Type");
        for (int i = 0; i < header_keys_length; i++){
            ESP_LOGI(TAG, "SET HEADER {%s:%s}", header_keys[i], header_values[i]);
            esp_http_client_set_header(client, header_keys[i], header_values[i]); 
        }
    }
    // Free body
    esp_http_client_set_post_field(client, '\0', 0);

    int status = 1;
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        status = 0;
    } else {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
            status = 0;
        } else {
            int data_read = esp_http_client_read_response(client, content, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOGI(TAG, "%s", content);
                if (esp_http_client_get_content_length(client) >= 400) status = 0;
            } else {
                ESP_LOGE(TAG, "Failed to read response");
                status = 0; 
            }
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    start_http_client();
    return status;
}



int post_api(const char *post_data, const char* api_address, esp_http_client_handle_t client, void** response) {
    esp_http_client_set_url(client, api_address);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64"", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client)); 
        esp_http_client_get_user_data(client, response);
        esp_http_client_close(client);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }    

    esp_http_client_cleanup(client);
    start_http_client();
    
    return 1;
}


void start_http_client() {
    const char* cert = read_from_spiffs("/spiffs/madbob-org-chain.pem");
    static char user_data[8192] = { 0 };

    esp_http_client_config_t config = { 
        .url= WEATHER_API,
        .cert_pem = cert,
        .timeout_ms = 10000,
        .buffer_size = 8192 * 2,
        .buffer_size_tx = 8192,
        .event_handler = _http_event_handler,
        .user_data = user_data
    }; 
    client_http = esp_http_client_init(&config);
}

