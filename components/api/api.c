#include <stdio.h>

// JSON parser
#include "jsmn.h"

#include "api.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#define MAX_HTTP_OUTPUT_BUFFER 1250

static const char *TAG = "HTTP_CLIENT";



int get_api(char *content, char* api_address, esp_http_client_handle_t client) {
    int content_length = 0;
    esp_http_client_set_url(client, api_address);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
        } else {
            int data_read = esp_http_client_read_response(client, content, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOGI(TAG, "%s", content);
            } else {
                ESP_LOGE(TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);
    return 1;
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
}

#include <dirent.h>

// Function to print the contents of a directory
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


const char* read_cert_from_spiffs() {
    init_spiffs();
    FILE* f = fopen("/spiffs/madbob-org-chain.pem", "r");  // Riferimento al file nel file system SPIFFS
    if (!f) {
        ESP_LOGE(TAG, "Failed to open certificate file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* cert = malloc(len + 1);
    fread(cert, 1, len, f);
    cert[len] = '\0';

    fclose(f);
    return cert;
}


void start_api() {
    char weather[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};   
    char gtt[2][200 + 1] = {0};
    
    
    const char* cert = read_cert_from_spiffs();

    esp_http_client_config_t config = { 
        .url= WEATHER_API,
        .disable_auto_redirect= false,
        .cert_pem = cert,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    char gtt_apis[2][100];
    sprintf(gtt_apis[0], "%s%s", GTT_API ,STOP_LEL);
    sprintf(gtt_apis[1], "%s%s", GTT_API, STOP_SND);

    get_api(gtt[0], gtt_apis[0], client);
    ESP_LOGI(TAG, "Chiamo lui: %s", gtt_apis[1]);  
    get_api(gtt[1], gtt_apis[1], client);

    get_api(weather, WEATHER_API, client);
    
    
    jsmn_parser p;
    jsmntok_t t[128];
    jsmn_init(&p);
    
 
    vTaskDelete(NULL);
    ESP_LOGI(TAG, "Chiamo lui: %s", WEATHER_API);  
}