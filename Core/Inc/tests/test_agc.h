/*
 * test_agc_f32.h
 *
 *  Created on: Nov 1, 2025
 *      Author: jaksty
 *
 *  Opis:
 *  Nagłówek testu AGC (float32) — symuluje kilka ramek
 *  o różnych amplitudach i sprawdza działanie automatycznej
 *  kontroli wzmocnienia.
 */

#ifndef TEST_AGC_F32_H
#define TEST_AGC_F32_H

#include "arm_math.h"
#include "voicecmd/vc_data_if.h"
#include "voicecmd/vc_agc.h"
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------
// Ustawienia testu
// --------------------------------------------------
// --------------------------------------------------
// Funkcje pomocnicze
// --------------------------------------------------

/**
 * @brief Generuje sinusoidę do bufora.
 * @param buf      bufor wyjściowy float32_t
 * @param N        liczba próbek
 * @param amplitude amplituda sygnału (0–1)
 * @param freq_hz  częstotliwość [Hz]
 */
void generate_sine(float32_t *buf, uint32_t N, float32_t amplitude, float32_t freq_hz);

/**
 * @brief Oblicza RMS sygnału (do diagnostyki).
 * @param buf bufor float32_t
 * @param N   liczba próbek
 * @return RMS sygnału
 */
float32_t compute_rms(const float32_t *buf, uint32_t N);

/**
 * @brief Uruchamia test AGC dla kilku ramek.
 * Wypisuje RMS przed i po przetworzeniu.
 */
void run_agc_test(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_AGC_F32_H */
