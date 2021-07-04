idf_component_register(SRCS "NAR_MQTT.c"
                    INCLUDE_DIRS "include"
                    REQUIRES protocol_examples_common mqtt nvs_flash
                    )