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
#include "esp_server.h"
#include "utils.h"
#include "calendar.h"

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
            // ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_url_query_len(req) + 1;
    char param[HTTP_QUERY_KEY_MAX_LEN], code[HTTP_QUERY_KEY_MAX_LEN] = {0}, scope[HTTP_QUERY_KEY_MAX_LEN] = {0}, id[3] = {0};

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
            if (httpd_query_key_value(buf, "state", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => id=%s", param);
                example_uri_decode(id, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", scope);
            }

        }
        free(buf);
    }


    // Ask for token and start routine of refreshing, OAUTH2 ROUTINE 
    token_management(code, scope, id);


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
    free(html_content);
    return ESP_OK;
}

static const httpd_uri_t home = {
    .uri = "/",
    .method = HTTP_GET,
    .handler   = home_handler,
    .user_ctx  = "OK!"
};

// GET METHODs
static esp_err_t get_info_handler(httpd_req_t *req) {
    nvs_handle_t nvs_handler;
    esp_err_t err;
    err = nvs_open("general_data", NVS_READONLY, &nvs_handler);

    char *city, *user_1, *user_2;
    char *city_key = "city";
    char *user_1_key = "user_1";
    char *user_2_key = "user_2";
    size_t total_size = 70; // Consider JSON construction

    if (get_from_nvs(nvs_handler, city_key, &city, &total_size) != ESP_OK) ESP_LOGE(TAG, "ERROR: get_info_handler");
    if (get_from_nvs(nvs_handler, user_1_key, &user_1, &total_size) != ESP_OK) {
       ESP_LOGE(TAG, "ERROR: get_info_handler");
    }
    if (get_from_nvs(nvs_handler, user_2_key, &user_2, &total_size) != ESP_OK) {
        ESP_LOGE(TAG, "ERROR: get_info_handler");
    }


    ESP_LOGI(TAG, "city: %s", city);
    ESP_LOGI(TAG, "us_1: %s", user_1);
    ESP_LOGI(TAG, "us_2: %s", user_2);

    // build the json
    char *json_buffer = (char *)malloc((total_size + strlen((const char *)OAUTH2_LINK)) * sizeof(char));

    snprintf(json_buffer, total_size + strlen((const char *) OAUTH2_LINK),
         "{\"oauth2_link\":\"%s\", \"city\":\"%s\", \"user_1\":\"%s\", \"user_2\":\"%s\"}",
         OAUTH2_LINK, city, user_1, user_2);

    ESP_LOGI(TAG, "INFO JSON: %s", json_buffer);

    httpd_resp_send(req, json_buffer, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);

    nvs_close(nvs_handler);

    free(city);
    free(json_buffer);
    free(user_1);
    free(user_2);
    return ESP_OK;
}

static const httpd_uri_t get_info = {
    .uri = "/get_info",
    .method = HTTP_GET,
    .handler   = get_info_handler,
    .user_ctx  = "OK!"
};

// SET METHODs
static esp_err_t set_city_handler(httpd_req_t *req) {
    nvs_handle_t nvs_handler;
    esp_err_t err;
    err = nvs_open("general_data", NVS_READWRITE, &nvs_handler);
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
    snprintf(url, 512, WEATHER_API, lat, longi);

    ESP_LOGI(TAG, "WEATHER API FOR %s: %s", city, url);
    
    
    err = nvs_set_str(nvs_handler, "city", city);
    err = nvs_set_str(nvs_handler, "w_api", url);

    err = nvs_commit(nvs_handler);

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

static esp_err_t set_users_handler(httpd_req_t *req) {
    nvs_handle_t nvs_handler;
    esp_err_t err;
    err = nvs_open("general_data", NVS_READWRITE, &nvs_handler);
    char data[500] = { '\0' };
    int ret = httpd_req_recv(req, data, 500);

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }


    char user_1[50] = { 0 }, user_2[50] = { 0 };
    decompose_json_dynamic_params(data, 2, "user_1", user_1, "user_2", user_2);

    ESP_LOGI(TAG, "USER 1: %s\tUSER 2: %s", user_1, user_2);
    
    if (strlen(user_1) == 0) {
        nvs_erase_key(nvs_handler, "user_1");
        nvs_erase_key(nvs_handler, "user_1_at");
        nvs_erase_key(nvs_handler, "user_1_rt");
        nvs_erase_key(nvs_handler, "user_1_ids");
    }
    else nvs_set_str(nvs_handler, "user_1", user_1);
    if (strlen(user_2) == 0) {
        nvs_erase_key(nvs_handler, "user_2");
        nvs_erase_key(nvs_handler, "user_2_at");
        nvs_erase_key(nvs_handler, "user_2_rt");
        nvs_erase_key(nvs_handler, "user_2_ids");
    }
    else nvs_set_str(nvs_handler, "user_2", user_2);

    err = nvs_commit(nvs_handler);

    nvs_close(nvs_handler);

    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);    

    return ESP_OK;
}

const httpd_uri_t set_users = {
    .uri = "/set_users",
    .method = HTTP_POST,
    .handler   = set_users_handler,
    .user_ctx  = "OK!",
};

// OTHER METHODs

static esp_err_t restart_esp_handler(httpd_req_t *req) {
    esp_restart();    

    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);    
    return ESP_OK;
}

const httpd_uri_t restart_esp = {
    .uri = "/restart_esp",
    .method = HTTP_GET,
    .handler   = restart_esp_handler,
    .user_ctx  = "OK!",
};

static httpd_handle_t start_webserver() {    
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &home);
        httpd_register_uri_handler(server, &key);
        httpd_register_uri_handler(server, &set_calendar);
        httpd_register_uri_handler(server, &set_city);
        httpd_register_uri_handler(server, &set_users);
        httpd_register_uri_handler(server, &restart_esp);
        httpd_register_uri_handler(server, &get_calendar);
        httpd_register_uri_handler(server, &user_validity);
        httpd_register_uri_handler(server, &get_info);
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