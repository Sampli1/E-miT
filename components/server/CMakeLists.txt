idf_component_register(SRCS "esp_server.c" "calendar.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        nvs_flash
                        esp_http_server
                        utils
                        client
                        )
