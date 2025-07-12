#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "env_var.h"


extern "C" {
    #include "esp_server.h"
    #include "spiffs_handler.h"
    #include "peripherals.h"
    #include "wifi.h"
    void app_main(void);
}

#include "screen.hpp"

static const char *TAG = "MAIN";


void app_main(void) {
    // NVS INITIALIZATION
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    // esp_log_level_set("httpd", ESP_LOG_VERBOSE);
    
    // Set WiFi and time synchronization
    wifi_and_time_init_sta();

    // Start spiffs memory
    init_spiffs();
     
    // Routine to start getting screen resources and display it
    xTaskCreate(start_screen, "SCREEN", 1024 * 30, NULL, 3, NULL);

    // Routine of peripherals
    xTaskCreate(start_gpio, "GPIO", 1024 * 2, NULL, 4, NULL);
}

