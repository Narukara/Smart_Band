idf_component_register(SRCS "MAX30102.c" "algorithm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES NAR_I2C NAR_GPIO
                    )