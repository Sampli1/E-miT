#include "calendar.h"

static esp_err_t get_calendar_handler(httpd_req_t *req) {
    return ESP_OK;
}

static esp_err_t set_calendar_handler(httpd_req_t *req) {
    return ESP_OK;
}

static esp_err_t user_validity_handler(httpd_req_t *req) {
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