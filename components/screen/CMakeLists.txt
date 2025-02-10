idf_component_register(SRCS "resources.cpp" "screen.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        client
                        server 
                        utils
                        CalEPD
                        Adafruit-GFX
                    )
