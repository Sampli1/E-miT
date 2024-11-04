#include "SPI.h"

#define PIN_MISO 25
#define PIN_MOSI 23
#define PIN_CLK  19
#define PIN_CS   22

void start_spi(void) {
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_MISO,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_device_interface_config_t devcfg = {
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 7,
    };

}
