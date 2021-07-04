/** \file algorithm.cpp ******************************************************
 *
 * Project: MAXREFDES117#
 * Filename: algorithm.cpp
 * Description: This module calculates the heart rate/SpO2 level
 *
 *
 * --------------------------------------------------------------------
 *
 * This code follows the following naming conventions:
 *
 * char              ch_pmod_value
 * char (array)      s_pmod_s_string[16]
 * float             f_pmod_value
 * int32_t           n_pmod_value
 * int32_t (array)   an_pmod_value[16]
 * int16_t           w_pmod_value
 * int16_t (array)   aw_pmod_value[16]
 * uint16_t          uw_pmod_value
 * uint16_t (array)  auw_pmod_value[16]
 * uint8_t           uch_pmod_value
 * uint8_t (array)   auch_pmod_buffer[16]
 * uint32_t          un_pmod_value
 * int32_t *         pn_pmod_value
 *
 * ------------------------------------------------------------------------- */
/*******************************************************************************
 * Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *******************************************************************************
 */

#include "esp_types.h"

#define FS 100
#define BUFFER_SIZE (FS * 5)
#define MA4_SIZE 4      // DO NOT CHANGE
#define HAMMING_SIZE 5  // DO NOT CHANGE
#define min(x, y) ((x) < (y) ? (x) : (y))

static const uint16_t auw_hamm[31] = {41, 276, 512, 276, 41};
// Hamm=  long16(512* hamming(5)');
// uch_spo2_table is computed as  -45.060*ratioAverage* ratioAverage + 30.354
// *ratioAverage + 94.845 ;
static int32_t an_dx[BUFFER_SIZE - MA4_SIZE];  // delta
static int32_t an_x[BUFFER_SIZE];              // ir

static void maxim_peaks_above_min_height(int32_t* pn_locs,
                                         int32_t* pn_npks,
                                         int32_t* pn_x,
                                         int32_t n_size,
                                         int32_t n_min_height);
static void maxim_remove_close_peaks(int32_t* pn_locs,
                                     int32_t* pn_npks,
                                     int32_t* pn_x,
                                     int32_t n_min_distance);
static void maxim_sort_ascend(int32_t* pn_x, int32_t n_size);
static void maxim_sort_indices_descend(int32_t* pn_x,
                                       int32_t* pn_indx,
                                       int32_t n_size);
static void maxim_find_peaks(int32_t* pn_locs,
                             int32_t* pn_npks,
                             int32_t* pn_x,
                             int32_t n_size,
                             int32_t n_min_height,
                             int32_t n_min_distance,
                             int32_t n_max_num);

/**
 * \brief        Calculate the heart rate and SpO2 level
 * \par          Details
 *               By detecting  peaks of PPG cycle and corresponding AC/DC of
 * red/infra-red signal, the ratio for the SPO2 is computed. Since this
 * algorithm is aiming for Arm M0/M3. formaula for SPO2 did not achieve the
 * accuracy due to register overflow. Thus, accurate SPO2 is precalculated and
 * save longo uch_spo2_table[] per each ratio.
 *
 * \param[in]    ir_buffer           - IR sensor data buffer
 * \param[in]    buffer_length      - IR sensor data buffer length
 * \param[out]    heart_rate          - Calculated heart rate value
 * \param[out]    hr_valid           - 1 if the calculated heart rate value
 * is valid
 *
 * \retval       None
 */
void maxim_heart_rate_saturation(uint32_t* ir_buffer,
                                 int32_t buffer_length,
                                 int32_t* heart_rate,
                                 int8_t* hr_valid) {
    uint32_t un_ir_mean;
    int32_t k, i, s, n_th1, n_npks, an_dx_peak_locs[15], n_peak_interval_sum;
    // remove DC of ir signal
    un_ir_mean = 0;
    for (k = 0; k < buffer_length; k++)
        un_ir_mean += ir_buffer[k];
    un_ir_mean = un_ir_mean / buffer_length;
    for (k = 0; k < buffer_length; k++)
        an_x[k] = ir_buffer[k] - un_ir_mean;

    // 4 pt Moving Average
    for (k = 0; k < BUFFER_SIZE - MA4_SIZE; k++) {
        an_x[k] = (an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3]) / 4;
    }

    // get difference of smoothed IR signal

    for (k = 0; k < BUFFER_SIZE - MA4_SIZE - 1; k++)
        an_dx[k] = (an_x[k + 1] - an_x[k]);

    // 2-pt Moving Average to an_dx
    for (k = 0; k < BUFFER_SIZE - MA4_SIZE - 2; k++) {
        an_dx[k] = (an_dx[k] + an_dx[k + 1]) / 2;
    }

    // hamming window
    // flip wave form so that we can detect valley with peak detector
    for (i = 0; i < BUFFER_SIZE - HAMMING_SIZE - MA4_SIZE - 2; i++) {
        s = 0;
        for (k = i; k < i + HAMMING_SIZE; k++) {
            s -= an_dx[k] * auw_hamm[k - i];
        }
        an_dx[i] = s / (int32_t)1146;  // divide by sum of auw_hamm
    }

    n_th1 = 0;  // threshold calculation
    for (k = 0; k < BUFFER_SIZE - HAMMING_SIZE; k++) {
        n_th1 += ((an_dx[k] > 0) ? an_dx[k] : ((int32_t)0 - an_dx[k]));
    }
    n_th1 = n_th1 / (BUFFER_SIZE - HAMMING_SIZE);
    // peak location is acutally index for sharpest location of raw signal since
    // we flipped the signal
    maxim_find_peaks(an_dx_peak_locs, &n_npks, an_dx,
                     BUFFER_SIZE - HAMMING_SIZE, n_th1, 8,
                     5);  // peak_height, peak_distance, max_num_peaks

    n_peak_interval_sum = 0;
    if (n_npks >= 2) {
        for (k = 1; k < n_npks; k++) {
            n_peak_interval_sum +=
                (an_dx_peak_locs[k] - an_dx_peak_locs[k - 1]);
        }
        n_peak_interval_sum = n_peak_interval_sum / (n_npks - 1);
        *heart_rate =
            (int32_t)(6000 / n_peak_interval_sum);  // beats per minutes
        *hr_valid = 1;
    } else {
        *heart_rate = -999;
        *hr_valid = 0;
    }
}

/**
 * \brief        Find peaks
 * \par          Details
 *               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at
 * least MIN_DISTANCE
 *
 * \retval       None
 */
static void maxim_find_peaks(int32_t* pn_locs,
                             int32_t* pn_npks,
                             int32_t* pn_x,
                             int32_t n_size,
                             int32_t n_min_height,
                             int32_t n_min_distance,
                             int32_t n_max_num) {
    maxim_peaks_above_min_height(pn_locs, pn_npks, pn_x, n_size, n_min_height);
    maxim_remove_close_peaks(pn_locs, pn_npks, pn_x, n_min_distance);
    *pn_npks = min(*pn_npks, n_max_num);
}

/**
 * \brief        Find peaks above n_min_height
 * \par          Details
 *               Find all peaks above MIN_HEIGHT
 *
 * \retval       None
 */
static void maxim_peaks_above_min_height(int32_t* pn_locs,
                                         int32_t* pn_npks,
                                         int32_t* pn_x,
                                         int32_t n_size,
                                         int32_t n_min_height) {
    int32_t i = 1, n_width;
    *pn_npks = 0;

    while (i < n_size - 1) {
        if (pn_x[i] > n_min_height &&
            pn_x[i] > pn_x[i - 1]) {  // find left edge of potential peaks
            n_width = 1;
            while (i + n_width < n_size &&
                   pn_x[i] == pn_x[i + n_width])  // find flat peaks
                n_width++;
            if (pn_x[i] > pn_x[i + n_width] &&
                (*pn_npks) < 15) {  // find right edge of peaks
                pn_locs[(*pn_npks)++] = i;
                // for flat peaks, peak location is left edge
                i += n_width + 1;
            } else
                i += n_width;
        } else
            i++;
    }
}

/**
 * \brief        Remove peaks
 * \par          Details
 *               Remove peaks separated by less than MIN_DISTANCE
 *
 * \retval       None
 */
static void maxim_remove_close_peaks(int32_t* pn_locs,
                                     int32_t* pn_npks,
                                     int32_t* pn_x,
                                     int32_t n_min_distance) {
    int32_t i, j, n_old_npks, n_dist;

    /* Order peaks from large to small */
    maxim_sort_indices_descend(pn_x, pn_locs, *pn_npks);

    for (i = -1; i < *pn_npks; i++) {
        n_old_npks = *pn_npks;
        *pn_npks = i + 1;
        for (j = i + 1; j < n_old_npks; j++) {
            n_dist =
                pn_locs[j] -
                (i == -1
                     ? -1
                     : pn_locs[i]);  // lag-zero peak of autocorr is at index -1
            if (n_dist > n_min_distance || n_dist < -n_min_distance)
                pn_locs[(*pn_npks)++] = pn_locs[j];
        }
    }

    // Resort indices longo ascending order
    maxim_sort_ascend(pn_locs, *pn_npks);
}

/**
 * \brief        Sort array
 * \par          Details
 *               Sort array in ascending order (insertion sort algorithm)
 *
 * \retval       None
 */
static void maxim_sort_ascend(int32_t* pn_x, int32_t n_size) {
    int32_t i, j, n_temp;
    for (i = 1; i < n_size; i++) {
        n_temp = pn_x[i];
        for (j = i; j > 0 && n_temp < pn_x[j - 1]; j--)
            pn_x[j] = pn_x[j - 1];
        pn_x[j] = n_temp;
    }
}

/**
 * \brief        Sort indices
 * \par          Details
 *               Sort indices according to descending order (insertion sort
 * algorithm)
 *
 * \retval       None
 */
static void maxim_sort_indices_descend(int32_t* pn_x,
                                       int32_t* pn_indx,
                                       int32_t n_size) {
    int32_t i, j, n_temp;
    for (i = 1; i < n_size; i++) {
        n_temp = pn_indx[i];
        for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j - 1]]; j--)
            pn_indx[j] = pn_indx[j - 1];
        pn_indx[j] = n_temp;
    }
}
