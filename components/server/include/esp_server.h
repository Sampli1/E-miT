#ifndef ESP_SERVER_H
#define ESP_SERVER_H

#include "server_utils.h"
#include "utils.h"
#include "client.h"
#include "oauth2.h"
#include "calendar.h"


void start_server(void *pvParameters);

void stop_server();

int is_server_open();

#endif // ESP_SERVER_H