idf_component_register(SRCS "peripherals.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        driver
                        screen
                        server
                        )
