idf_component_register(SRCS "client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        esp_http_client
                        env_var
                        spiffs
                        esp-tls
                        utils
                    )
