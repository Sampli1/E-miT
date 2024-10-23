idf_component_register(SRCS "utils.c" "server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        nvs_flash
                        esp_http_server
                        api)
