idf_component_register(SRCS "utils.c" "jsmn.c" "spiffs_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        spiffs)
