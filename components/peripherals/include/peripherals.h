#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/queue.h"

#include "screen.hpp"

void start_gpio(void *pvParameters);

#endif // PERIPHERALS_H