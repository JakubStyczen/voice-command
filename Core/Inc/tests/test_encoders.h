/*
 * test_encoders.h
 *
 *  Created on: Nov 8, 2025
 *      Author: jaksty
 *
 *  Opis:
 *  Nagłówek testu funkcji compress_data() dla różnych kodeków
 *  (PCM16 i IMA-ADPCM) - symuluje kompresję ramek audio.
 */

#ifndef TEST_ENCODERS_H
#define TEST_ENCODERS_H

#include "arm_math.h"
#include "voicecmd/vc_data_if.h"
#include "voicecmd/vc_encoders.h"
#include "voicecmd/vc_decoders.h"
#include "voicecmd/vc_filters.h"
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------
// Ustawienia testu
// --------------------------------------------------
#define TEST_FS_HZ          16000
#define TEST_FRAME_SAMPLES  320
#define TEST_PI_F           3.14159265359f

// --------------------------------------------------
// Funkcje pomocnicze
// --------------------------------------------------

/**
 * @brief Generuje sinusoidę do bufora float32_t.
 * @param buf      bufor wyjściowy float32_t
 * @param N        liczba próbek
 * @param amplitude amplituda sygnału (0–1)
 * @param freq_hz  częstotliwość [Hz]
 */
void test_generate_sine_f32(float32_t *buf, uint32_t N, float32_t amplitude, float32_t freq_hz);

/**
 * @brief Generuje szum biały do bufora float32_t.
 * @param buf      bufor wyjściowy float32_t
 * @param N        liczba próbek
 * @param amplitude amplituda szumu (0–1)
 */
void test_generate_white_noise_f32(float32_t *buf, uint32_t N, float32_t amplitude);

/**
 * @brief Oblicza RMS sygnału float32_t (do diagnostyki).
 * @param buf bufor float32_t
 * @param N   liczba próbek
 * @return RMS sygnału
 */
float32_t test_compute_rms_f32(const float32_t *buf, uint32_t N);

/**
 * @brief Test kompresji PCM16 - pokazuje metadane i rozmiar danych.
 */
void test_pcm16_compression(void);

/**
 * @brief Test kompresji IMA-ADPCM - pokazuje współczynnik kompresji.
 */
void test_ima_adpcm_compression(void);

void test_g722_compression(void);

/**
 * @brief Uruchamia wszystkie testy kodeków.
 */
void run_encoders_test(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_ENCODERS_H */
