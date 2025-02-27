#ifndef NVS_UTILS_H
#define NVS_UTILS_H

#include "nvs.h"
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t get_from_nvs(nvs_handle_t nvs_handler, char *key, char **val, size_t *total_size);

#ifdef __cplusplus
}
#endif


#endif // NVS_UTILS_H