#ifndef OAUTH2_H
#define OAUTH2_H

#include "client.h"
#include "nvs_utils.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void token_management(char *code, char *scope, char *id);

int refresh_token_managment(int id, char *response);

esp_err_t get_api_oauth2(char *content, char *api_address, nvs_handle_t NVS, int id);

#ifdef __cplusplus
}
#endif

#endif // OAUTH2_H