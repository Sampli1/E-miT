#include "peripherals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static uint8_t httpd_status = 0;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    gpio_intr_disable(gpio_num); 
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static uint8_t toggle = 0;
void blink() {
    gpio_set_level(PIN_HTTPD_LED, toggle);
    toggle = !toggle;
}


static void gpio_task(void* arg) {
    uint32_t io_num;

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));

            if (gpio_get_level(io_num)) {
                // Refresh e-paper info and deactivate it for 1 minutes

                gpio_intr_disable(io_num);

                if (!httpd_status) {
                    // Enable server
                    gpio_set_level(PIN_HTTPD_LED, 1);
                    httpd_status = 1;

                    // Routine to start a server (HTTPd)
                    xTaskCreate(start_server, "SERVER", 1024 * 15, NULL, 2, NULL);

                }
                else {
                    // Kill server
                    gpio_set_level(PIN_HTTPD_LED, 0);
                    httpd_status = 0;
                    stop_server();
                }
                

                // 1 seconds of cooldown
                vTaskDelay(pdMS_TO_TICKS(5000));
                printf("[GPIO] Ready\n");

                gpio_set_level(PIN_LED, 0);

                gpio_intr_enable(io_num); 
            }
        }
    }
}

void pwr_epaper_on() {
    gpio_set_level(EPAPER_PWR, 1);
}

void pwr_epaper_off() {
    gpio_set_level(EPAPER_PWR, 0);
}

void start_gpio(void *pvParameters) {
    printf("[GPIO] Start config\n");

    // PIN_LED
    gpio_config_t led_conf = {};
    led_conf.intr_type = GPIO_INTR_DISABLE;
    led_conf.mode = GPIO_MODE_OUTPUT;
    led_conf.pin_bit_mask = (1ULL << PIN_LED);
    led_conf.pull_down_en = 0;
    led_conf.pull_up_en = 0;
    gpio_config(&led_conf);

    // PIN_WAKEUP
    esp_sleep_enable_ext0_wakeup(PIN_WAKEUP, 1);

    // PIN_E_PAPER
    gpio_config_t epaper_pwr_conf = {};
    epaper_pwr_conf.mode = GPIO_MODE_OUTPUT;
    epaper_pwr_conf.pin_bit_mask = (1ULL << EPAPER_PWR);
    epaper_pwr_conf.pull_down_en = 0;
    epaper_pwr_conf.pull_up_en = 0;
    gpio_config(&epaper_pwr_conf);


    // PIN_HTTPD
    gpio_config_t httpd_conf = {};
    httpd_conf.intr_type = GPIO_INTR_POSEDGE;
    httpd_conf.mode = GPIO_MODE_INPUT;
    httpd_conf.pin_bit_mask = (1ULL << PIN_HTTPD);
    httpd_conf.pull_down_en = 0;
    httpd_conf.pull_up_en = 0;
    gpio_config(&httpd_conf);


    // PIN_HTTPD_LED
    gpio_config_t httpd_led_conf = {};
    httpd_led_conf.mode = GPIO_MODE_OUTPUT;
    httpd_led_conf.pin_bit_mask = (1ULL << PIN_HTTPD_LED);
    httpd_led_conf.pull_down_en = 0;
    httpd_led_conf.pull_up_en = 0;
    gpio_config(&httpd_led_conf);

    // Queue creation for gpio events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(PIN_HTTPD, gpio_isr_handler, (void*) PIN_HTTPD);

    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
    
    vTaskDelete(NULL);
}
