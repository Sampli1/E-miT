#ifndef CLIENT_H_
#define CLIENT_H_


#include "nvs_flash.h"
#include "esp_http_client.h"
#include "env_var.h"
#include "freertos/FreeRTOS.h"

#define MAX_POST_BODY_LENGTH 400
#define MAX_POST_RES_LENGTH 400
#define MAX_HTTP_OUTPUT_BUFFER 5000
#define MAX_SPIFFS_PATH_LENGTH 100

extern SemaphoreHandle_t client_http_mutex;
extern esp_http_client_handle_t client_http;


#ifdef __cplusplus
extern "C" {
#endif
int post_api(const char *post_data, const char* api_address, esp_http_client_handle_t client, void** response);

int get_api(char *content, const char* api_address, char **header_keys, char **header_values, int header_keys_length);

void load_cert();

void start_http_client();
#ifdef __cplusplus
}
#endif

#endif // CLIENT_H_