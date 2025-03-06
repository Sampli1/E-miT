
#include "nvs_utils.h"

static const char* TAG = "NVS_UTILS"; 

esp_err_t get_from_nvs(nvs_handle_t nvs_handler, char *key, char **val, size_t *total_size) {
    size_t required_size;
    esp_err_t err;

    err = nvs_get_str(nvs_handler, key, NULL, &required_size);
    // ESP_LOGI(TAG, "required size: %zu", required_size);
    
    if (err == ESP_OK) {
        (*val) = (char *) malloc(required_size * sizeof(char));
        if (total_size) *total_size += required_size;
        err = nvs_get_str(nvs_handler, key, *val, &required_size);
        // ESP_LOGI(TAG, "%s: %s", key, *val);
    }
    else {
        (*val) = calloc(10, sizeof(char));
        sprintf(*val, "NULL");
    }
    return err;
}