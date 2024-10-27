idf_component_register(SRCS "api.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        esp_http_client
                        env_var
                        spiffs
                        esp-tls
                    )
