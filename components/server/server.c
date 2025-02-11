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
#include "spiffs_handler.h"

#include "server_utils.h"
#include "server.h"
#include "utils.h"

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
    char param[HTTP_QUERY_KEY_MAX_LEN], code[HTTP_QUERY_KEY_MAX_LEN] = {0}, scope[HTTP_QUERY_KEY_MAX_LEN] = {0};

    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "code", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => code=%s", param);
                example_uri_decode(code, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", code);
            }
            if (httpd_query_key_value(buf, "scope", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => scope=%s", param);
                example_uri_decode(scope, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
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
    char *home_string = "/spiffs/home.html";
    const char *html_content = read_from_spiffs(home_string);
    
    httpd_resp_send(req, html_content, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static const httpd_uri_t home = {
    .uri = "/",
    .method = HTTP_GET,
    .handler   = home_handler,
    .user_ctx  = "OK!"
};


static esp_err_t get_info_handler(httpd_req_t *req) {
    httpd_resp_send(req, OAUTH2_LINK, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static const httpd_uri_t get_info = {
    .uri = "/get_info",
    .method = HTTP_GET,
    .handler   = get_info_handler,
    .user_ctx  = "OK!"
};

static esp_err_t get_city_handler(httpd_req_t *req) {
    nvs_handle_t nvs_handler;
    esp_err_t err;
    err = nvs_open("nvs", NVS_READONLY, &nvs_handler);

    size_t required_size;
    err = nvs_get_str(nvs_handler, "city", NULL, &required_size);
    if (err == ESP_OK) {
        char *city = malloc(required_size);
        err = nvs_get_str(nvs_handler, "city", city, &required_size);
        
        ESP_LOGI(TAG, "CITY_NAME: %s", city);

        if (err == ESP_OK) {
            httpd_resp_send(req, city, HTTPD_RESP_USE_STRLEN);
        } else {
            httpd_resp_send(req, NULL, HTTPD_RESP_USE_STRLEN);
        }
        free(city);
    } else {
        httpd_resp_send(req, NULL, HTTPD_RESP_USE_STRLEN);
    }

    nvs_close(nvs_handler);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static const httpd_uri_t get_city = {
    .uri = "/get_city",
    .method = HTTP_GET,
    .handler   = get_city_handler,
    .user_ctx  = "OK!"
};

void get_weather_url(char url[512], int url_size, char *lat, char *lon) {
    snprintf(url, url_size, WEATHER_API, lat, lon);
}

static esp_err_t set_city_handler(httpd_req_t *req) {
    nvs_handle_t nvs_handler;
    esp_err_t err;
    err = nvs_open("nvs", NVS_READWRITE, &nvs_handler);
    char data[500] = { '\0' };
    int ret = httpd_req_recv(req, data, 500);

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }


    char city[50], lat[10], longi[10], url[512] = { 0 };
    decompose_json_dynamic_params(data, 3, "name", city, "lat", lat, "long", longi);
    get_weather_url(url, 512, lat, longi);

    ESP_LOGI(TAG, "WEATHER API FOR %s: %s", city, url);
    
    
    err = nvs_set_str(nvs_handler, "city", city);
    err = nvs_set_str(nvs_handler, "w_api", url);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Errore nella scrittura dell'API: %s", esp_err_to_name(err));
    }

    err = nvs_commit(nvs_handler);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Errore nel commit dei dati su NVS: %s", esp_err_to_name(err));
    }


    nvs_close(nvs_handler);

    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);    
    return ESP_OK;
}

const httpd_uri_t set_city = {
    .uri = "/set_city",
    .method = HTTP_POST,
    .handler   = set_city_handler,
    .user_ctx  = "OK!",
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
        httpd_register_uri_handler(server, &set_city);
        httpd_register_uri_handler(server, &get_calendar);
        httpd_register_uri_handler(server, &user_validity);
        httpd_register_uri_handler(server, &get_info);
        httpd_register_uri_handler(server, &get_city);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}


void start_server(void *pvParameters) {
    httpd_handle_t server = start_webserver();
    
    while (server) {
        sleep(5);
    }
}