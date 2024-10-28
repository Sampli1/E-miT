#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "esp_event.h"
#include "esp_check.h"

#include "server.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  128


static const char *TAG = "SERVER";




static esp_err_t key_get_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_url_query_len(req) + 1;
    char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], code[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0}, scope[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};

    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "code", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => code=%s", param);
                example_uri_decode(code, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", code);
            }
            if (httpd_query_key_value(buf, "scope", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => scope=%s", param);
                example_uri_decode(scope, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", scope);
            }
        }
        free(buf);
    }


    // Ask for token and start routine of refreshing, OAUTH2 ROUTINE 
    token_management(code, scope);


    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

static const httpd_uri_t key = {
    .uri       = "/key",
    .method    = HTTP_GET,
    .handler   = key_get_handler,
    .user_ctx  = "OK!"
};

// Home route
static esp_err_t home_handler(httpd_req_t *req) {
    
    FILE *html_fp = fopen("/spiffs/home.html", "r");

    if (html_fp == NULL) ESP_LOGI(TAG, "Vaffanculo");

    char link[] = OAUTH2_LINK;
    char resp_str[450];
    sprintf(resp_str, "%s%s%s", "<a href=\"", link, "\"> Link per google </a>");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static const httpd_uri_t home = {
    .uri = "/",
    .method = HTTP_GET,
    .handler   = home_handler,
    .user_ctx  = "OK!"
};


static httpd_handle_t start_webserver() {    
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
     
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &home);
        httpd_register_uri_handler(server, &key);
        httpd_register_uri_handler(server, &set_calendar);
        httpd_register_uri_handler(server, &get_calendar);
        httpd_register_uri_handler(server, &user_validity);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}


void start_server() {
    httpd_handle_t server = start_webserver();

    while (server) {
        sleep(5);
    }
}