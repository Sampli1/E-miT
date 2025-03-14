#include "peripherals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define PIN_LED 26
#define PIN_SENSOR 27

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;


static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    gpio_intr_disable(gpio_num); 
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void gpio_task(void* arg) {
    uint32_t io_num;
    uint8_t toggle = 0;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));

            // I have only a GPIO interrupt
            gpio_set_level(PIN_LED, toggle++);
            toggle = !toggle;

            gpio_intr_enable(io_num); 
        }
    }
}

void start_gpio(void *pvParameters) {
    // PIN_LED
    gpio_config_t led_conf = {};
    led_conf.intr_type = GPIO_INTR_DISABLE;
    led_conf.mode = GPIO_MODE_OUTPUT;
    led_conf.pin_bit_mask = (1ULL << PIN_LED);
    led_conf.pull_down_en = 0;
    led_conf.pull_up_en = 0;
    gpio_config(&led_conf);

    // PIN_SENSOR
    gpio_config_t sensor_conf = {};
    sensor_conf.intr_type = GPIO_INTR_POSEDGE;
    sensor_conf.mode = GPIO_MODE_INPUT;
    sensor_conf.pin_bit_mask = (1ULL << PIN_SENSOR);
    sensor_conf.pull_down_en = 0;
    sensor_conf.pull_up_en = 0;
    gpio_config(&sensor_conf);

    // Queue creation for gpio events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(PIN_SENSOR, gpio_isr_handler, (void*) PIN_SENSOR);

    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
    
    vTaskDelete(NULL);
}
