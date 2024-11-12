idf_component_register(SRCS "screen.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        client
                        server 
                        CalEPD
                        Adafruit-GFX
                    )
