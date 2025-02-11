#include "peripherals.h"
#include <portmacro.h>

#define PIN_MISO 25
#define PIN_MOSI 23
#define PIN_CLK  19
#define PIN_CS   22

void start_gpio(void *pvParameters) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << 32);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    int cnt = 0;
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(32, cnt % 2);
    }
}
