#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "spiffs_handler.h"

#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_check.h"

static const char *TAG = "SPIFFS";


void print_directory_contents(const char *base_path) {
    DIR *dir = opendir(base_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", base_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            ESP_LOGI(TAG, "Found: %s/%s", base_path, entry->d_name);
        }
    }
    closedir(dir);
}

void init_spiffs() {
   esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,  
        .max_files = 5,          
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to mount SPIFFS (error %d)", ret);
        }
        return;
    }

    ESP_LOGI(TAG, "SPIFFS mounted successfully");
    const char* spiffs_string = "/spiffs";
    print_directory_contents(spiffs_string);
}



const char* read_from_spiffs(char *filename) {
    FILE* f = fopen(filename, "r");  // Riferimento al file nel file system SPIFFS
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc(len + 1);
    fread(content, 1, len, f);
    content[len] = '\0';

    fclose(f);
    return content;
}

int write_on_spiffs(char *filename, char *content) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) return -1;
    return fprintf(f, content);
}
