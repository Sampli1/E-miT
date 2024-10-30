#include "calendar.h"
#include "spiffs_handler.h"
#include "utils.h"
#include "server_utils.h"

static const char *TAG = "CALENDAR";

static esp_err_t get_calendar_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    char name[HTTP_QUERY_KEY_MAX_LEN] = {0}, param[HTTP_QUERY_KEY_MAX_LEN] = {0};

    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "name", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => code=%s", param);
                example_uri_decode(name, param, strnlen(param, HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", name);
            }
        }
        free(buf);
    }

    const char* file = read_from_spiffs("/spiffs/calendars.txt");
    if (file == NULL) return ESP_FAIL;

    char *found = strstr(file, name);    
    if (found == NULL) return ESP_FAIL;
    char *token = strtok(found, "\n");

    if (token != NULL) httpd_resp_send(req, token + strlen(name) + 1, HTTPD_RESP_USE_STRLEN);
    else httpd_resp_send(req, found + strlen(name) + 1, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t set_calendar_handler(httpd_req_t *req) {
    char content[50];
    int ret = httpd_req_recv(req, content, 50);


    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    

    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t user_validity_handler(httpd_req_t *req) {
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

const httpd_uri_t get_calendar = {
    .uri = "/get_calendar",
    .method = HTTP_GET,
    .handler   = get_calendar_handler,
    .user_ctx  = "OK!"
};


const httpd_uri_t set_calendar = {
    .uri = "/set_calendar",
    .method = HTTP_POST,
    .handler   = set_calendar_handler,
    .user_ctx  = "OK!"
};

const httpd_uri_t user_validity = {
    .uri = "/check_user",
    .method = HTTP_GET,
    .handler   = user_validity_handler,
    .user_ctx  = "OK!"
};