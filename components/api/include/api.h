#ifndef API_H_
#define API_H_


#include "esp_http_client.h"
#include "env_var.h"

#define MAX_POST_BODY_LENGTH 400
#define MAX_POST_RES_LENGTH 400
#define MAX_HTTP_OUTPUT_BUFFER 1800

extern esp_http_client_handle_t client_http;



int post_api(char *post_data, char* api_address, esp_http_client_handle_t client, void** response);

int get_api(char *content, char* api_address, esp_http_client_handle_t client, char **header_keys, char **header_values, int header_keys_length);

void start_get_requests();

void start_api();


#endif // API_H_