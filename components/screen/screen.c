#include <stdio.h>
#include "screen.h"

static const char *TAG = "SCREEN";

void get_requests() {
    char weather[MAX_HTTP_OUTPUT_BUFFER + 1] = {'\0'};   
    char *gtt[2];
    gtt[0] = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    gtt[1] = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    char gtt_apis[2][100];
    sprintf(gtt_apis[0], "%s%s", GTT_API, STOP_LEL);
    sprintf(gtt_apis[1], "%s%s", GTT_API, STOP_SND);

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Chiamo lui: %s", gtt_apis[0]);  
        get_api(gtt[0], gtt_apis[0], client_http, NULL, NULL, 0);
        xSemaphoreGive(client_http_mutex);
    }

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Chiamo lui: %s", gtt_apis[1]);  
        get_api(gtt[1], gtt_apis[1], client_http, NULL, NULL, 0);
        xSemaphoreGive(client_http_mutex);
    }

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Chiamo lui: %s", WEATHER_API);  
        get_api(weather, WEATHER_API, client_http, NULL, NULL, 0);
        xSemaphoreGive(client_http_mutex);
    }

}


void start_screen() {
    get_requests();
    vTaskDelete(NULL);
}
