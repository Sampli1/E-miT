#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/queue.h"

#include "esp_server.h"
#include "screen.hpp"

#define PIN_HTTPD 34
#define PIN_LED 26
#define PIN_HTTPD_LED 27
#define EPAPER_PWR 32
#define PIN_WAKEUP GPIO_NUM_33

void start_gpio(void *pvParameters);

void pwr_epaper_on();

void pwr_epaper_off();

void blink();

#endif // PERIPHERALS_H