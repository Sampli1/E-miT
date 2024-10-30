idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "."
                    REQUIRES
                        client
                        server
                        screen
                        esp_wifi
                        nvs_flash
                        utils
                        )
