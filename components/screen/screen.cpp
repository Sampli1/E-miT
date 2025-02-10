#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string>

#include "screen.hpp"
#include "icons_128x128.h"
#include "gdew075T7.h"
#include "Fonts/FreeMonoBold12pt7b.h"


#define MAX_GTT_INFO 3


static const char *TAG = "SCREEN";


extern "C" {
    #include "client.h"
    #include "server.h"  
    #include "utils.h"
};


EpdSpi io;
Gdew075T7 display(io);

void delay(uint32_t millis) { 
    vTaskDelay(millis / portTICK_PERIOD_MS);
}

const unsigned char *write_weather_icon(int value) {
    switch (value) {
        case 0:
            return _00_128x128;
        case 1:
            return _01_128x128;
        case 2:
            return _02_128x128;
        case 3:
            return _03_128x128;
        case 45:
            return _45_128x128;
        case 48:
            return _48_128x128;
        case 51:
            return _51_128x128;
        case 53:
            return _53_128x128;
        case 55:
            return _55_128x128;
        case 61:
            return _61_128x128;
        case 63:
            return _63_128x128;
        case 65:
            return _65_128x128;
        case 66:
            return _66_128x128;
        case 67:
            return _67_128x128;
        case 71:
            return _71_128x128;
        case 73:
            return _73_128x128;
        case 75:
            return _75_128x128;
        case 77:
            return _77_128x128;
        case 80:
            return _80_128x128;
        case 81:
            return _81_128x128;
        case 82:
            return _82_128x128;
        case 95:
            return _95_128x128;
        case 96:
            return _96_128x128;
        case 99:
            return _96_128x128;
        default:
            return _100_128x128;
    }
}

void print_gtt_info(char *gtt, int col1_x, int top_y, int row_spacing, char *line_val) {
    char *json_vec[20];
    int length;
    char *hour = (char *) calloc(100, sizeof(char));
    char *line = (char *) calloc(100, sizeof(char));
    for (int i = 0; i < 20; i++) json_vec[i] = (char*) calloc(MAX_POST_RES_LENGTH, sizeof(char));

    from_string_to_json_string_vec(gtt, json_vec, &length);

    int j = 0;
    for (int i = 0; i < length; i++) {
        decompose_json_dynamic_params(json_vec[i], 2, "hour", hour, "line", line);
        if (strcmp(line, line_val) == 0 && j < MAX_GTT_INFO) {
            display.setCursor(col1_x, top_y + (j + 1) * row_spacing);
            display.print(hour);
            j++;
        }
    }

    for (int i = 0; i < 20; i++) free(json_vec[i]);
    free(hour);
    free(line);
}


void write_gtt() {
    int col1_x = 50;
    int col2_x = 250;
    int top_y = 300;
    int row_spacing = 30;
    
    display.setCursor(col1_x, top_y);
    display.print("4N");

    display.setCursor(col2_x, top_y);
    display.print("10");

    char *gtt[2];
    const char *gtt_apis[2];

    gtt_apis[0] = GTT_API STOP_LEL;
    gtt_apis[1] = GTT_API STOP_SND;
    gtt[0] = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    gtt[1] = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));

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

    ESP_LOGI(TAG, "Lelle %s", gtt[0]);
    ESP_LOGI(TAG, "Sandro %s", gtt[1]);

    char* el = (char *) "4N";
    print_gtt_info(gtt[0], col1_x, top_y, row_spacing, el);
    el = (char *) "10";
    print_gtt_info(gtt[1], col2_x, top_y, row_spacing, el);

}


void write_meteo(struct tm timeinfo) {
    char *weather = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    int top_y = 30;
    int top_x = 30;
    int row_spacing = 30;
    char *hourly = (char *) calloc(1000, sizeof(char));
    char *temperatures = (char *) calloc(200, sizeof(char));
    char *precipitations = (char *) calloc(200, sizeof(char));
    char *wheaters = (char *) calloc(200, sizeof(char));
    int dim;
    int temperatures_values[24];
    int precipitations_values[24];
    int wheaters_values[24];

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Chiamo lui: %s", WEATHER_API);  
        get_api(weather, WEATHER_API, client_http, NULL, NULL, 0);
        xSemaphoreGive(client_http_mutex);
    }
 

    // Handle JSON data => please, avoid using C for future projects
    decompose_json_dynamic_params(weather, 1, "hourly", hourly);
    decompose_json_dynamic_params(hourly, 3, "temperature_2m", temperatures, "precipitation_probability", precipitations, "weather_code", wheaters);    
    from_string_to_int_array(temperatures, temperatures_values, &dim);
    from_string_to_int_array(precipitations, precipitations_values, &dim);
    from_string_to_int_array(wheaters, wheaters_values, &dim);

    int icon_val = wheaters_values[timeinfo.tm_hour];
    const unsigned char *icon = write_weather_icon(icon_val);
    int temperature = temperatures_values[timeinfo.tm_hour];

    ESP_LOGI(TAG,"temperature: %d", temperature);

    display.drawBitmap(top_x, top_y, icon, 128, 128, EPD_BLACK);
    display.setCursor(top_x + 128, top_y);
    
    display.print("Torino");
    display.setCursor(top_x + 128, top_y + row_spacing);
    char temperature_to_display[30] = { '\0' };
    sprintf(temperature_to_display ,"Temperatura: %d °C", temperature);
    display.print(temperature_to_display);
}

void write_calendar() {

}


void write_time(struct tm timeinfo) {
    char strftime_buf[64];
    display.setCursor(12, 12);
    strftime(strftime_buf, sizeof(strftime_buf), "%d-%m-%Y %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "Date: %s", strftime_buf);
    display.print(strftime_buf);
}

void start_screen(void *pvParameters) {
    display.init(true);

    delay(5000);

    // Config
    display.setTextSize(1);
    display.setTextColor(EPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    display.fillScreen(EPD_WHITE);

    // Shared data
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);



    write_time(timeinfo);
    write_meteo(timeinfo);
    write_gtt();
    write_calendar();

    display.update();

    vTaskDelete(NULL);
}
