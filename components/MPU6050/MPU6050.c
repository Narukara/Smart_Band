/**
 * @file MPU6050.c
 * @brief MPU6050 API for esp32, based on motion_driver_test.c and official
 * driver
 * @author Narukara
 * @date 2021.2
 */
#include "esp_err.h"
#include "esp_log.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

#define DEFAULT_MPU_HZ (100)

static const char* TAG = "MPU6050";

static enum { ready, running, error } status = ready;

#define fuse()                                 \
    ({                                         \
        status = error;                        \
        ESP_LOGE(TAG, "fuse at %d", __LINE__); \
    })

static const int8_t gyro_orientation[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};

static uint16_t inv_row_2_scale(const int8_t* row) {
    uint16_t b;
    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;  // error
    return b;
}

static inline uint16_t inv_orientation_matrix_to_scalar(const int8_t* mtx) {
    uint16_t scalar;
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

// static void tap_cb(uint8_t direction, uint8_t count) {
//     ESP_LOGI(TAG, "tap_cd");
// }

// static void android_orient_cb(uint8_t orientation) {
//     ESP_LOGI(TAG, "android_orient_cd");
// }

void MPU6050_init() {
    if (status != ready) {
        return;
    }

    struct int_param_s int_param;
    if (mpu_init(&int_param)) {
        fuse();
        return;
    }

    if (mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL)) {
        fuse();
        return;
    }

    if (mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL)) {
        fuse();
        return;
    }

    if (mpu_set_sample_rate(DEFAULT_MPU_HZ)) {
        fuse();
        return;
    }

    if (dmp_load_motion_driver_firmware()) {
        fuse();
        return;
    }

    if (dmp_set_orientation(
            inv_orientation_matrix_to_scalar(gyro_orientation))) {
        fuse();
        return;
    }

    // if (dmp_register_tap_cb(tap_cb)) {
    //     ESP_ERROR_CHECK_WITHOUT_ABORT(ESP_FAIL);
    //     return ESP_FAIL;
    // }

    // if (dmp_register_android_orient_cb(android_orient_cb)) {
    //     ESP_ERROR_CHECK_WITHOUT_ABORT(ESP_FAIL);
    //     return ESP_FAIL;
    // }

    if (dmp_enable_feature(DMP_FEATURE_TAP)) {
        fuse();
        return;
    }

    if (dmp_set_fifo_rate(DEFAULT_MPU_HZ)) {
        fuse();
        return;
    }

    if (mpu_set_dmp_state(1)) {
        fuse();
        return;
    }
    status = running;
    ESP_LOGI(TAG, "mpu6050 init");
}

/**
 * @return step. 999999 if failed
 */
unsigned long MPU6050_get_step() {
    if (status != running) {
        return 999999;
    }
    unsigned long count;
    if (dmp_get_pedometer_step_count(&count)) {
        fuse();
        return 999999;
    }
    return count;
}

void MPU6050_set_step(unsigned long count) {
    if (status != running) {
        return;
    }
    if (dmp_set_pedometer_step_count(count)) {
        fuse();
    }
}