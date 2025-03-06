#include <stdio.h>
#include <malloc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string>
#include <nvs_flash.h>


#include "screen.hpp"
#include "icons_128x128.h"
#include "generic_icons.h"
#include "gdew075T7.h"
#include "nvs_utils.h"


#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/Tiny3x3a2pt7b.h"
#include "Fonts/FreeMonoBold9pt7b.h"
#include "Fonts/FreeMonoBold12pt7b.h"
#include "Fonts/FreeMonoBold18pt7b.h"


#define MAX_GTT_INFO 2 
#define MAX_EVENTS_PER_USER 10
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define FIRST_COLUMN 300
#define SECOND_COLUMN 550
#define HOURS_IN_A_DAY 24


static const char *TAG = "SCREEN";


extern "C" {
    #include "client.h"
    #include "esp_server.h"  
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
    int col1_x = 30;
    int col2_x = 155;
    int top_y = 400;
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

        // ?
        ESP_LOGI(TAG, "FREE HEAP before api call: %"PRIu32, esp_get_free_heap_size());
        get_api(gtt[0], gtt_apis[0], NULL, NULL, 0);
        // ?
        ESP_LOGI(TAG, "FREE HEAP after api call: %"PRIu32, esp_get_free_heap_size());
        xSemaphoreGive(client_http_mutex);
    }

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Chiamo lui: %s", gtt_apis[1]);  
        // ?
        ESP_LOGI(TAG, "FREE HEAP before api call: %"PRIu32, esp_get_free_heap_size());
        get_api(gtt[1], gtt_apis[1], NULL, NULL, 0);
        // ?
        ESP_LOGI(TAG, "FREE HEAP after api call: %"PRIu32, esp_get_free_heap_size());
        xSemaphoreGive(client_http_mutex);
    }


    ESP_LOGI(TAG, "Lelle %s", gtt[0]);
    ESP_LOGI(TAG, "Sandro %s", gtt[1]);

    char* el = (char *) "4N";
    print_gtt_info(gtt[0], col1_x, top_y, row_spacing, el);
    el = (char *) "10";
    print_gtt_info(gtt[1], col2_x, top_y, row_spacing, el);

    free(gtt[0]);
    free(gtt[1]);
}

void get_city_info(char weather_api[512], char city_name[50], nvs_handle_t nvs_handler) {
    esp_err_t err;

    size_t value;
    err = nvs_get_str(nvs_handler, "city", city_name, &value);
    if (value > 50) {
        ESP_LOGE(TAG, "DO YOU LIVE IN AOTHURIAEIGNIEUBNGRIrgeoiqegi?");
        exit(1);
    }

    size_t value_len = 512; 
    err = nvs_get_str(nvs_handler, "w_api", weather_api, &value_len);

    ESP_LOGI(TAG, "WHEATHER API FOR %s: %s", city_name, weather_api);
}

void write_meteo(struct tm timeinfo, nvs_handle nvs_handler) {
    char *weather = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    char *hourly = (char *) calloc(1000, sizeof(char));
    char *temperatures = (char *) calloc(200, sizeof(char));
    char *precipitations = (char *) calloc(200, sizeof(char));
    char *wheaters = (char *) calloc(200, sizeof(char));
    int dim;
    int temperatures_values[24];
    int precipitations_values[24];
    int wheaters_values[24];

    char weather_api[512] = {'\0'}, city_name[50];
    get_city_info(weather_api, city_name, nvs_handler);

    if (xSemaphoreTake(client_http_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Chiamo lui: %s", weather_api);  
        get_api(weather, weather_api, NULL, NULL, 0);
        xSemaphoreGive(client_http_mutex);
    }

    // Handle JSON data => please, avoid using C for future projects
    decompose_json_dynamic_params(weather, 1, "hourly", hourly);
    decompose_json_dynamic_params(hourly, 3, "temperature_2m", temperatures, "precipitation_probability", precipitations, "weather_code", wheaters);    
    from_string_to_int_array(temperatures, temperatures_values, &dim);
    from_string_to_int_array(precipitations, precipitations_values, &dim);
    from_string_to_int_array(wheaters, wheaters_values, &dim);

    // Meteo icons and values 

    int icon_val = wheaters_values[timeinfo.tm_hour];
    const unsigned char *icon = write_weather_icon(icon_val);
    int temperature = temperatures_values[timeinfo.tm_hour];

    // City name
    int16_t x_pos = 0, y_pos = 100, x_end, y_end, centeredX;
    uint16_t w, h;
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextSize(1);
    display.getTextBounds(city_name, x_pos, y_pos, &x_end, &y_end, &w, &h);
    if (x_end >= FIRST_COLUMN) {
        // Long city name... trying to reduce dimension:
        display.setFont(&FreeMonoBold12pt7b);
        display.setTextSize(1);
        display.getTextBounds(city_name, x_pos, y_pos, &x_end, &y_end, &w, &h);
    }
    if (x_end < FIRST_COLUMN) {
        centeredX = (FIRST_COLUMN - w)/ 2;
        display.setCursor(centeredX, y_pos);
        display.print(city_name);
    }
    uint16_t y_temperature = display.getCursorY();

    // Temperature & Climate
    display.setCursor(0, 0);
    char temperature_to_display[30] = { '\0' };
    sprintf(temperature_to_display ,"%d", temperature);
    uint16_t sum = 0;
    int16_t image_size = 128;
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextSize(1);
    display.getTextBounds(temperature_to_display, x_pos, y_pos, &x_end, &y_end, &w, &h); 
    sum += w;
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextSize(1);
    display.getTextBounds(" o", x_pos, y_pos, &x_end, &y_end, &w, &h); 
    sum += w;
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextSize(1);
    display.getTextBounds("C", x_pos, y_pos, &x_end, &y_end, &w, &h); 
    sum += w;
    sum += image_size; 
    sum += 25; // Inner padding
    centeredX = (FIRST_COLUMN - sum)/ 2;
    y_temperature += image_size/2;     

    display.setCursor(centeredX, y_temperature);
    display.print(temperature_to_display);
    display.setFont(&FreeMonoBold9pt7b);
    display.print(" ");
    x_pos = display.getCursorX();
    y_pos = display.getCursorY();
    display.setCursor(x_pos, y_pos - 12);
    display.print("o");
    x_pos = display.getCursorX();
    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(x_pos, y_pos);
    display.print("C");
    x_pos = display.getCursorX();
    display.drawBitmap(x_pos + 25, y_pos - image_size/2, icon, image_size, image_size, EPD_BLACK);


    // Rain probability chart => chart of 240 px width and 50 px heighs
    display.setFont(&FreeSans9pt7b);
    display.setTextSize(1);
    const int x_base = 30;
    const int y_base = 350;
    const int chart_width = 150;
    const int chart_height = 100;
    char buffer[10] = { '\0' };

    for (int w = 0; w < 3; w++) display.drawLine(x_base, y_base - w, x_base + chart_width , y_base - w, EPD_BLACK);

    for (int w = 0; w < 3; w++) display.drawLine(x_base - w, y_base, x_base - w, y_base - chart_height, EPD_BLACK);

    // Y-axes
    for (int i = 0; i <= 100; i += 25) {
        y_pos = y_base - ((i / 100.0f) * chart_height);
        display.setCursor(x_base - 30, y_pos - 3);
        sprintf(buffer, "%d", i);
        display.print(buffer);
        display.drawLine(x_base - 4, y_pos, x_base, y_pos, EPD_BLACK);
    }

    // X-axes
    for (int i = 0; i < 24; i += 6) {
        x_pos = x_base + (i * 10);
        display.setCursor(x_pos - 15, y_base + 15);
        sprintf(buffer, "%d:00", i);
        display.print(buffer);
        display.drawLine(x_pos, y_base + 4, x_pos, y_base, EPD_BLACK);
    }
    
    int ex_value;
    for (int i = 0; i < HOURS_IN_A_DAY; i++) {
        int value = (int) ((precipitations_values[i] / 100.0f) * chart_height);
        display.drawPixel(x_base - (i * 10), y_base - value, EPD_BLACK);
        if (i > 0) for (int w = 0; w < 3; w++) display.drawLine(x_base + ((i - 1) * 10), y_base - ex_value - w, x_base + (i * 10), y_base - value - w, EPD_BLACK);
        ex_value = value;
    }

    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
}


typedef struct {
    uint8_t h;
    uint8_t m;
} Date;

typedef struct {
    char title[50];
    Date start;
    Date end;
} Event;


Date parse_datetime(const char *datetime) {
    Date d = {0, 0};  // Inizializza con 0
    int hour, minute;

    if (sscanf(datetime, "%*d-%*d-%*dT%d:%d", &hour, &minute) == 2) {
        d.h = (uint8_t)hour;
        d.m = (uint8_t)minute;
    }
    else ESP_LOGE(TAG, "ERROR IN PARSING DATE"); 

    return d;
}


void decompose_events_to_struct(char *raw_events, Event *events, uint8_t *events_number) {

    char *events_dec = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    char *json_events[MAX_EVENTS_PER_USER];
    char *start_date = (char *) calloc(200, sizeof(char));
    char *end_date = (char *) calloc(200, sizeof(char));
    char *raw_date = (char *) calloc(30, sizeof(char));
    int events_length = -1;
    for (int i = 0; i < MAX_EVENTS_PER_USER; i++) json_events[i] = (char *) calloc(500, sizeof(char));
    
    
    decompose_json_dynamic_params(raw_events, 1, "items", events_dec); 
    from_string_to_json_string_vec(events_dec, json_events, &events_length);
    
    if (events_length > 10) {
        ESP_LOGW(TAG, "WARNING: TOO MUCH EVENTS, ONLY THE FIRST %d EVENTS WILL BE SHOWN", MAX_EVENTS_PER_USER);
        events_length = 10;
    }

    *events_number += events_length;


    for (int i = 0; i < events_length; i++) {
        decompose_json_dynamic_params(json_events[i], 3, "summary", events[i].title, "start", start_date, "end", end_date);
        if (start_date[0] == '\0' || end_date[0] == '\0') {
            ESP_LOGE(TAG, "%s NOT DECOMPOSED", json_events[i]);
            continue;
        }
        decompose_json_dynamic_params(start_date, 1, "dateTime", raw_date);
        if (raw_date[0] == '\0') {
            ESP_LOGE(TAG, "%s\n DATE START DOES NOT TRANSLATED", json_events[i]);
            continue;
        }
        events[i].start = parse_datetime(raw_date);
        decompose_json_dynamic_params(end_date, 1, "dateTime", raw_date);
        if (raw_date[0] == '\0') {
            ESP_LOGE(TAG, "%s\n DATE END DOES NOT TRANSLATED", json_events[i]);
            continue;
        }
        events[i].end = parse_datetime(raw_date);
    }

    for (int i = 0; i < MAX_EVENTS_PER_USER; i++) free(json_events[i]);
    free(raw_date);
    free(end_date);
    free(start_date);
    free(events_dec);
}


int compare_events(const void *a, const void *b) {
    Event *eventA = *(Event **)a;
    Event *eventB = *(Event **)b;

    if (eventA->start.h != eventB->start.h) 
        return eventA->start.h - eventB->start.h;
    
    return eventA->start.m - eventB->start.m;
}

void sort_events(Event events[2][MAX_CALENDARS_EPAPER][MAX_EVENTS_PER_USER], Event *sorted_events[2][MAX_EVENTS_PER_USER]) {
    for (int user = 0; user < 2; user++) {
        int event_count = 0;

        for (int cal = 0; cal < MAX_CALENDARS_EPAPER; cal++) {
            for (int evt = 0; evt < MAX_EVENTS_PER_USER; evt++) {
                if (events[user][cal][evt].title[0] != '\0') {
                    if (event_count < MAX_EVENTS_PER_USER) { // Prevent overflow
                        sorted_events[user][event_count] = &events[user][cal][evt];
                        event_count++;
                    }
                }
            }
        }

        qsort(sorted_events[user], event_count, sizeof(Event *), compare_events);

        for (int i = event_count; i < MAX_EVENTS_PER_USER; i++) {
            sorted_events[user][i] = NULL;
        }
    }
}

// ChatGPT makes good print functions 
void printEvents(Event events[2][MAX_CALENDARS_EPAPER][MAX_EVENTS_PER_USER]) {
    for (int user = 0; user < 2; user++) {
        printf("\n==============================\n");
        printf("         User %d\n", user + 1);
        printf("==============================\n");

        for (int cal = 0; cal < MAX_CALENDARS_EPAPER; cal++) {
            printf("\n  --- Calendar %d ---\n", cal + 1);

            for (int ev = 0; ev < MAX_EVENTS_PER_USER; ev++) {
                Event e = events[user][cal][ev];

                if (e.title[0] == '\0') continue;

                printf("  📌 %s\n", e.title);
                printf("     ⏳ %02d:%02d - %02d:%02d\n",
                       e.start.h, e.start.m, e.end.h, e.end.m);
            }
        }
    }
}


void init_events(Event event[2][MAX_CALENDARS_EPAPER][MAX_EVENTS_PER_USER]) {
    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < MAX_CALENDARS_EPAPER; j++) {
            for (size_t k = 0; k < MAX_EVENTS_PER_USER; k++) {
                event[i][j][k].title[0] = '\0';
            }
        }
    }
}


void print_sorted_events(Event *(sorted_events[2][MAX_EVENTS_PER_USER])) {
    for (int user = 0; user < 2; user++) {
        printf("User %d's sorted events:\n", user + 1);
        for (int i = 0; i < MAX_EVENTS_PER_USER; i++) {
            if (sorted_events[user][i] != NULL) {
                printf("  [%02d:%02d] %s\n", sorted_events[user][i]->start.h, sorted_events[user][i]->start.m, sorted_events[user][i]->title);
            }
        }
        printf("\n");
    }
}


void draw_event_block(Event event, int16_t pos_x, int16_t pos_y, int block_width, int block_height) {
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextSize(1);

    int padding = 5;
    int16_t x_end, y_end, centeredX;
    uint16_t w, h;
    
    int titleX = pos_x + padding;
    int titleY = pos_y + 30;
    
    display.setCursor(titleX, titleY);
    display.getTextBounds(event.title, pos_x, pos_y, &x_end, &y_end, &w, &h);
    display.print(event.title);

    char timeBuffer[20];

    sprintf(timeBuffer, "%02d:%02d-%02d:%02d", event.start.h, event.start.m, event.end.h, event.end.m);
    display.setFont(&FreeMonoBold9pt7b);
    display.getTextBounds(timeBuffer, pos_x, pos_y, &x_end, &y_end, &w, &h);
    int timeX = pos_x + block_width - padding - w;
    display.setCursor(timeX, pos_y + padding + 10);
    display.print(timeBuffer);

    display.drawRect(pos_x, pos_y, block_width, block_height, EPD_BLACK);
}



void calendar_2(nvs_handle_t nvs_handler, struct tm timeinfo) {
    char *url = (char *) calloc(strlen(CALENDAR_ELEMENTS_LINK) + 1024 + 400, sizeof(char)); // 400 may be sufficient for dates
    char *res = (char *) calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
    char *calendars; 
    char **calendars_list;
    
    Event events[2][MAX_CALENDARS_EPAPER][MAX_EVENTS_PER_USER];
    init_events(events);

    uint8_t events_number[2] = {0};

    int calendar_list_len;
    int16_t x_pos = 0, y_pos = 26, x_end, y_end, centeredX;
    uint16_t w = 0, h, x_center, y_center;
    char *name;

    char key[17];
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    
    display.drawLine(SECOND_COLUMN, 0, SECOND_COLUMN, SCREEN_HEIGHT, EPD_BLACK);

    // Write names    
    for (int i = 1; i < 3; i++) {
        // Print names
        sprintf(key, "user_%d", i);
        get_from_nvs(nvs_handler, key, &name, NULL);
        ESP_LOGI(TAG, "%s %s", key, name);
        display.getTextBounds(name, x_pos, y_pos, &x_end, &y_end, &w, &h);

        x_center = FIRST_COLUMN + (((SECOND_COLUMN - FIRST_COLUMN) * (2*i - 1)) - w)/2;

        ESP_LOGI(TAG, "X_Center %d", x_center);
        display.setCursor(x_center, y_pos);
        display.println(name);
        if (name) free(name);
    }

    y_pos = display.getCursorY() - 5;

    display.drawLine(FIRST_COLUMN, y_pos, SCREEN_WIDTH, y_pos, EPD_BLACK);

    // Get all calendars and do correct calculations
    uint16_t max_height = SCREEN_HEIGHT - y_pos;


    // ?
    ESP_LOGI(TAG, "FREE HEAP before first loop: %"PRIu32, esp_get_free_heap_size());

    for (int i = 1; i < 3; i++) {
        sprintf(key, "user_%d_ids", i);
        if (get_from_nvs(nvs_handler, key, &calendars, NULL) != ESP_OK) continue;

        from_string_to_string_array(calendars, &calendars_list, &calendar_list_len);

        if (calendar_list_len > MAX_CALENDARS_EPAPER) {
            ESP_LOGE(TAG, "FATAL: TOO MANY CALENDARs for user_%d", i);
            esp_restart();
        }

        for (int j = 0; j < calendar_list_len; j++) {
            sprintf(url, CALENDAR_ELEMENTS_LINK, calendars_list[j], timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
            get_api_oauth2(res, url, nvs_handler, i);
            ESP_LOGI(TAG, "RES: %s", res);
            
            if (res[0] == '\0') continue;

            decompose_events_to_struct(res, events[i - 1][j], &events_number[i]);
            memset(res, 0, MAX_HTTP_OUTPUT_BUFFER);
        }

        free(calendars);
        free_string_array(calendars_list, calendar_list_len);
    }

    // ?
    ESP_LOGI(TAG, "FREE HEAP after first loop: %"PRIu32, esp_get_free_heap_size());


    ESP_LOGI(TAG, "GET THE FIRST 10 EVENTS PER USER AND MAKE PROPORTION OF SCREEN");

    Event *sorted_events[2][MAX_EVENTS_PER_USER] = { NULL };

    sort_events(events, sorted_events);

    print_sorted_events(sorted_events);

    x_pos = 0;
    y_pos +=10;
    int block_width = 230;

    // ?
    ESP_LOGI(TAG, "FREE HEAP before second loop: %"PRIu32, esp_get_free_heap_size());
    for (int i = 0; i < 2; i++) {
        x_pos = i == 0 ? SECOND_COLUMN - block_width : SECOND_COLUMN;

        for (int j = 0; j < MAX_EVENTS_PER_USER; j++) {
        
            if (sorted_events[i][j] != NULL) {
                draw_event_block(*(sorted_events[i][j]), x_pos, y_pos, block_width, 70);
                y_pos += 80;
            }
        }
    }

    // ?
    ESP_LOGI(TAG, "FREE HEAP after second loop: %"PRIu32, esp_get_free_heap_size());

    free(res);
    free(url);
}

void calendar_0() {
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    
    int16_t y_pos = 20, x_pos = 0, x_end, y_end;
    uint16_t w, h, qr_dim = 256;
    char text[50] = "Necessito input, inquadrare:";
    display.getTextBounds(text, x_pos, y_pos, &x_end, &y_end, &w, &h);
    uint16_t average_y = (SCREEN_HEIGHT - (h + qr_dim + 30)) / 2; // 30 stands for space between qr_code and text 
    uint16_t average_x = FIRST_COLUMN + (SCREEN_WIDTH - FIRST_COLUMN - w)/2, average_x_qr = FIRST_COLUMN + (SCREEN_WIDTH - FIRST_COLUMN - qr_dim)/2;

    display.setCursor(average_x, average_y);
    display.print(text);

    display.setCursor(average_x_qr, average_y + 30);
    display.drawBitmap(average_x_qr, average_y + 30, qr_code, qr_dim, qr_dim, EPD_BLACK);

}

void write_calendar(struct tm timeinfo, nvs_handle_t nvs_handler) {
    // For each user and for each calendar of that user 
    char *name;
    char key[17];
    size_t length;
    uint8_t users_number = 0;
    nvs_open("general_data", NVS_READONLY, &nvs_handler);

    for (int i = 1; i < 3; i++) {
        // Count names
        sprintf(key, "user_%d", i);
        get_from_nvs(nvs_handler, key, &name, &length);
        if (strcmp(name, "NULL") != 0) users_number++;
        if (name) free(name);
    }

    switch (users_number) {
        case 0: calendar_0(); break;
        case 1: calendar_0(); break;
        case 2: {
            // ?
            ESP_LOGI(TAG, "FREE HEAP before calendar_2(): %"PRIu32, esp_get_free_heap_size());
            calendar_2(nvs_handler, timeinfo);
            // ?
            ESP_LOGI(TAG, "FREE HEAP after calendar_2(): %"PRIu32, esp_get_free_heap_size());
            break;
        }
    }

}


void write_time(struct tm timeinfo) {
    char time_buf[64], date_buf[64];
    const char *giorni_settimana_it[] = { "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};

    int16_t x_pos = 0, y_pos = 22, x_end, y_end, centeredX;
    uint16_t w = 0, h;
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextSize(1);

    strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);
    display.getTextBounds(time_buf, x_pos, y_pos, &x_end, &y_end, &w, &h);

    centeredX = (FIRST_COLUMN - w)/2;
    display.setCursor(centeredX, y_pos);
    display.println(time_buf);

    // Day 
    y_pos = display.getCursorY();
    const char *giorno_settimana = giorni_settimana_it[timeinfo.tm_wday];
    snprintf(date_buf, sizeof(date_buf), "%s %02d/%02d/%d", giorno_settimana, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.getTextBounds(date_buf, x_pos, y_pos, &x_end, &y_end, &w, &h);
    centeredX = (FIRST_COLUMN - w)/2;
    display.setCursor(centeredX, y_pos);
    display.println(date_buf);    
}


void write_separator() {
    // vertical separator 
    display.writeLine(FIRST_COLUMN, 0, FIRST_COLUMN, SCREEN_HEIGHT, EPD_BLACK); 
}

void start_screen(void *pvParameters) {
    display.init(true);

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

    // Open NVS
    nvs_handle_t nvs_handler;
    nvs_open("general_data", NVS_READONLY, &nvs_handler);

    write_separator();
    write_gtt();
    write_time(timeinfo);
    write_meteo(timeinfo, nvs_handler);
    write_calendar(timeinfo, nvs_handler);


    ESP_LOGI(TAG, "Writing success!");

    display.update();
    nvs_close(nvs_handler);
    vTaskDelete(NULL);
}
