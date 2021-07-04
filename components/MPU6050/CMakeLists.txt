idf_component_register(SRCS "inv_mpu.c" "inv_mpu_dmp_motion_driver.c" "MPU6050.c"
                    INCLUDE_DIRS "include"
                    REQUIRES NAR_I2C
                    )