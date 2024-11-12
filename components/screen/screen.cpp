#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "screen.hpp"
#include "zeb.h"
#include "gdew075T7.h"

static const char *TAG = "SCREEN";


extern "C" {
    #include "client.h"
    #include "server.h"  

    // void get_requests() {
        // char weather[MAX_HTTP_OUTPUT_BUFFER + 1] = {'\0'};   
        // char *gtt[2];
        // gtt[0] = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
        // gtt[1] = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
        // char gtt_apis[2][100];
        // sprintf(gtt_apis[0], "%s%s", GTT_API, STOP_LEL);
        // sprintf(gtt_apis[1], "%s%s", GTT_API, STOP_SND);

        // if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
            // ESP_LOGI(TAG, "Chiamo lui: %s", gtt_apis[0]);  
            // get_api(gtt[0], gtt_apis[0], client_http, NULL, NULL, 0);
            // xSemaphoreGive(client_http_mutex);
        // }

        // if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
            // ESP_LOGI(TAG, "Chiamo lui: %s", gtt_apis[1]);  
            // get_api(gtt[1], gtt_apis[1], client_http, NULL, NULL, 0);
            // xSemaphoreGive(client_http_mutex);
        // }

        // if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
            // ESP_LOGI(TAG, "Chiamo lui: %s", WEATHER_API);  
            // get_api(weather, WEATHER_API, client_http, NULL, NULL, 0);
            // xSemaphoreGive(client_http_mutex);
        // }

    // }
};


EpdSpi io;
Gdew075T7 display(io);

void delay(uint32_t millis) { vTaskDelay(millis / portTICK_PERIOD_MS); }


void start_screen(void *pvParameters) {
    // get_requests();

    display.init(true);

    delay(2500);

    uint16_t rectW = 0;

    display.fillScreen(EPD_WHITE);
    display.fillCircle(rectW, display.height()/2,90, EPD_BLACK);
    display.drawCircle(rectW*2, display.height()/2,90, EPD_BLACK);
    display.fillCircle(rectW*3, display.height()/2,90, EPD_BLACK);
    
    display.update();
    
    delay(2500);

    display.fillScreen(EPD_WHITE);


    printf("EPD width: %d height: %d\n\n", display.width(), display.height());
    
    delay(1000);
    display.fillScreen(EPD_WHITE);

    
    delay(1500);
    display.fillScreen(EPD_WHITE);
    printf("display: We are done with the demo\n");


    
    display.drawBitmap(0, 0, epd_zeb, 225, 225, EPD_BLACK);

    display.update();

    vTaskDelete(NULL);
}
