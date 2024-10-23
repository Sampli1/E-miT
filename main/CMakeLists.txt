idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "."
                    REQUIRES
                        "api"
                        server
                        esp_wifi
                        nvs_flash
                        )
