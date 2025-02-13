#ifndef CLIENT_H_
#define CLIENT_H_


#include "esp_http_client.h"
#include "env_var.h"
#include "freertos/FreeRTOS.h"

#define MAX_POST_BODY_LENGTH 400
#define MAX_POST_RES_LENGTH 400
#define MAX_HTTP_OUTPUT_BUFFER 5000

extern SemaphoreHandle_t client_http_mutex;
extern esp_http_client_handle_t client_http;


int post_api(const char *post_data, const char* api_address, esp_http_client_handle_t client, void** response);

int get_api(char *content, const char* api_address, esp_http_client_handle_t client, char **header_keys, char **header_values, int header_keys_length);

void start_get_requests(void *http_client_mutex);

void start_http_client();



#endif // CLIENT_H_