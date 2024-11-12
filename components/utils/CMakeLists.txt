idf_component_register(SRCS "wifi.c" "utils.c" "jsmn.c" "spiffs_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        nvs_flash
                        esp_wifi
                        spiffs
                        env_var)
