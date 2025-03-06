idf_component_register(SRCS "oauth2.c" "client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        esp_http_client
                        env_var
                        spiffs
                        nvs_flash
                        esp-tls
                        utils
                    )
