#ifndef CALENDAR_H
#define CALENDAR_H


#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"


extern const httpd_uri_t get_calendar;
extern const httpd_uri_t set_calendar;
extern const httpd_uri_t user_validity;

esp_err_t get_from_nvs(nvs_handle_t nvs_handler, char *key, char **val, size_t *total_size);

#endif // CALENDAR_H