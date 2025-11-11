/*
 * test_encoders.c
 *
 *  Created on: Nov 8, 2025
 *      Author: jaksty
 *
 *  Testy funkcji compress_data() dla różnych kodeków:
 *  - PCM16 (bez kompresji)
 *  - IMA-ADPCM (kompresja 4:1)
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <tests/test_encoders.h>

// --------------------------------------------------
// Funkcje pomocnicze
// --------------------------------------------------

void test_generate_sine_f32(float32_t *buf, uint32_t N, float32_t amplitude, float32_t freq_hz)
{
    for (uint32_t i = 0; i < N; i++) {
        float angle = 2.0f * TEST_PI_F * freq_hz * ((float)i / TEST_FS_HZ);
        buf[i] = amplitude * sinf(angle);
    }
}

void test_generate_white_noise_f32(float32_t *buf, uint32_t N, float32_t amplitude)
{
    for (uint32_t i = 0; i < N; i++) {
        // Prosta implementacja szumu pseudolosowego
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        buf[i] = amplitude * noise;
    }
}

float32_t test_compute_rms_f32(const float32_t *buf, uint32_t N)
{
    float32_t rms;
    arm_rms_f32(buf, N, &rms);
    return rms;
}

// --------------------------------------------------
// Testy kompresji
// --------------------------------------------------

void test_pcm16_compression(void)
{
    // Przygotowanie metadanych PCM
    vc_pcm_meta_t pcm_meta = {
        .frame_idx = 1,
        .sample_rate_hz = TEST_FS_HZ,
        .channels = 1,
        .len_samples = TEST_FRAME_SAMPLES
    };
    
    // Konfiguracja kodera PCM16
    vc_enc_cfg_t cfg = {
        .codec = VC_CODEC_PCM16,
        .samples_per_block = 0  // nie dotyczy PCM16
    };
    
    // Generowanie sygnału testowego (1 kHz sinus)
    float32_t test_signal[TEST_FRAME_SAMPLES];
    test_generate_sine_f32(test_signal, TEST_FRAME_SAMPLES, 400.0f, 1000.0f);
    
    float32_t rms_input = test_compute_rms_f32(test_signal, TEST_FRAME_SAMPLES);
    
    // Test kompresji PCM16
    compress_data(&pcm_meta, test_signal, &cfg);
    
    // Informacje o PCM16
    vc_stream_meta_t pcm_meta_info;
    vc_meta_pcm16_make(&pcm_meta_info);
    
    uint32_t pcm_frame_size = TEST_FRAME_SAMPLES * 2;  // 2 bajty na próbkę
}

void test_ima_adpcm_compression(void)
{
    // Przygotowanie metadanych PCM
    vc_pcm_meta_t pcm_meta = {
        .frame_idx = 2,
        .sample_rate_hz = TEST_FS_HZ,
        .channels = 1,
        .len_samples = TEST_FRAME_SAMPLES
    };
    
    // Konfiguracja kodera IMA-ADPCM
    vc_enc_cfg_t cfg = {
        .codec = VC_CODEC_IMA_ADPCM,
        .samples_per_block = TEST_FRAME_SAMPLES
    };
    
    // Generowanie sygnału testowego (mowa syntetyczna - mix częstotliwości)
    float32_t test_signal[TEST_FRAME_SAMPLES];
    float32_t temp_signal[TEST_FRAME_SAMPLES];
    
    // Składowa 300 Hz (podstawowa częstotliwość)
    test_generate_sine_f32(test_signal, TEST_FRAME_SAMPLES, 300.0f, 300.0f);
    
    // Dodanie harmonicznej 900 Hz
    test_generate_sine_f32(temp_signal, TEST_FRAME_SAMPLES, 200.0f, 900.0f);
    arm_add_f32(test_signal, temp_signal, test_signal, TEST_FRAME_SAMPLES);
    
    // Dodanie harmonicznej 1500 Hz
    test_generate_sine_f32(temp_signal, TEST_FRAME_SAMPLES, 100.0f, 1500.0f);
    arm_add_f32(test_signal, temp_signal, test_signal, TEST_FRAME_SAMPLES);
    
    float32_t rms_input = test_compute_rms_f32(test_signal, TEST_FRAME_SAMPLES);
    
    // Test kompresji IMA-ADPCM
    compress_data(&pcm_meta, test_signal, &cfg);
    
    // Informacje o IMA-ADPCM
    vc_stream_meta_t adpcm_meta_info;
    vc_meta_ima_adpcm_make(&adpcm_meta_info);
    
    uint32_t pcm_frame_size = TEST_FRAME_SAMPLES * 2;  // PCM16: 640 bajtów
    uint32_t adpcm_frame_size = adpcm_meta_info.block_align;  // IMA-ADPCM: 164 bajty
    float compression_ratio = (float)pcm_frame_size / (float)adpcm_frame_size;
}

void test_g722_compression(void){
    // g722_init_64k_enc();
    g722_init_64k_dec();
    
    // Przygotowanie metadanych PCM
    vc_pcm_meta_t pcm_meta = {
        .frame_idx = 2,
        .sample_rate_hz = TEST_FS_HZ,
        .channels = 1,
        .len_samples = TEST_FRAME_SAMPLES
    };
    
    // Konfiguracja kodera IMA-ADPCM
    vc_enc_cfg_t cfg = {
        .codec = VC_CODEC_G722,
        .samples_per_block = TEST_FRAME_SAMPLES
    };
    
    // Generowanie sygnału testowego (mowa syntetyczna - mix częstotliwości)
    float32_t test_signal[TEST_FRAME_SAMPLES];
    float32_t temp_signal[TEST_FRAME_SAMPLES];
    
    // Składowa 300 Hz (podstawowa częstotliwość)
    test_generate_sine_f32(test_signal, TEST_FRAME_SAMPLES, 300.0f, 300.0f);
    
    // Dodanie harmonicznej 900 Hz
    test_generate_sine_f32(temp_signal, TEST_FRAME_SAMPLES, 200.0f, 900.0f);
    arm_add_f32(test_signal, temp_signal, test_signal, TEST_FRAME_SAMPLES);
    
    // Dodanie harmonicznej 1500 Hz
    test_generate_sine_f32(temp_signal, TEST_FRAME_SAMPLES, 100.0f, 1500.0f);
    arm_add_f32(test_signal, temp_signal, test_signal, TEST_FRAME_SAMPLES);
    
    float32_t rms_input = test_compute_rms_f32(test_signal, TEST_FRAME_SAMPLES);
    
    // Test kompresji IMA-ADPCM
    compress_data(&pcm_meta, test_signal, &cfg);
    
    // Informacje o IMA-ADPCM
    vc_stream_meta_t adpcm_meta_info;
    vc_meta_ima_adpcm_make(&adpcm_meta_info);
    
    uint32_t pcm_frame_size = TEST_FRAME_SAMPLES * 2;  // PCM16: 640 bajtów
    uint32_t adpcm_frame_size = adpcm_meta_info.block_align;  // IMA-ADPCM: 164 bajty
    float compression_ratio = (float)pcm_frame_size / (float)adpcm_frame_size;
    
    // g722_deinit_enc();
    g722_deinit_dec();
}

void run_encoders_test(void)
{
    // Inicjalizacja generatora liczb losowych
    srand(12345);
    
    // Uruchom wszystkie testy
    test_g722_compression();
    // test_pcm16_compression();
    // test_ima_adpcm_compression();
}
