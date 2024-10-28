#ifndef CALENDAR_H
#define CALENDAR_H


#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "esp_event.h"
#include "esp_check.h"

extern const httpd_uri_t get_calendar;
extern const httpd_uri_t set_calendar;
extern const httpd_uri_t user_validity;

#endif // CALENDAR_H