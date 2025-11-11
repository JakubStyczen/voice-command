/*
 * Test funkcji vc_convert_int16_to_float32 z 320 próbkami
 * 
 * Ten kod generuje różne sygnały testowe (320 próbek każdy)
 * i pokazuje działanie funkcji konwersji int16_t -> float32_t
 */

#include "voicecmd/vc_filters.h"
#include "voicecmd/vc_data_if.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

typedef struct VC_PACKED {
    uint32_t frame_idx;         /* ++ co ramkę (diagnostyka) */
    uint32_t sample_rate_hz;    /* = VC_FS_HZ */
    uint16_t channels;          /* = 1 */
    uint16_t len_samples;       /* zwykle = VC_FRAME_SAMPLES (320) */
    int16_t * samples[VC_FRAME_SAMPLES];           /* int16_t[len_samples] */
} frame_pointer;

// Test filtracji HPF z 320 próbkami
void test_conversion_320_samples(void)
{
    // Metadane PCM
    vc_pcm_meta_t pcm_meta = {
        .frame_idx = 1,
        .sample_rate_hz = 16000,
        .channels = 1,
        .len_samples = 320
    };
    
    // Tablice danych
    int16_t input_pcm[320];      // dane wejściowe PCM
    float32_t output_dsp[320];   // dane wyjściowe po HPF
    
    // Generowanie sygnału testowego 200 Hz (powinien przejść przez HPF 100 Hz z mniejszym tłumieniem)
    for (int i = 0; i < 320; i++) {
        float angle = 2.0f * M_PI * 200.0f * (float)i / 16000.0f;
        input_pcm[i] = (int16_t)(32767.0f * sin(angle));
    }
    
    // Obliczenie RMS przed filtracją
    float32_t temp_float[320];
    vc_convert_int16_to_float32(input_pcm, temp_float, 320);

    // float32_t rms_before_int16;
    // arm_rms_f32(input_pcm, 320, &rms_before_int16);

    float32_t rms_before;
    arm_rms_f32(temp_float, 320, &rms_before);
    
    // Inicjalizacja i filtrowanie HPF
    vc_hpf_init_f32();
    vc_hpf_process_frame_f32(&pcm_meta, input_pcm, output_dsp);
    
    // Obliczenie RMS po filtracji
    float32_t rms_after;
    arm_rms_f32(output_dsp, 320, &rms_after);
    
    // Obliczenie tłumienia w dB
    float32_t attenuation_db = 20.0f * log10f(rms_after / rms_before);
    
    // Test z sygnałem wysokiej częstotliwości (1000 Hz - powinien przejść)
    for (int i = 0; i < 320; i++) {
        float angle = 2.0f * M_PI * 1000.0f * (float)i / 16000.0f;
        input_pcm[i] = (int16_t)(32767.0f * sin(angle));
    }
    
    vc_convert_int16_to_float32(input_pcm, temp_float, 320);
    arm_rms_f32(temp_float, 320, &rms_before);
    
    vc_hpf_process_frame_f32(&pcm_meta, input_pcm, output_dsp);
    arm_rms_f32(output_dsp, 320, &rms_after);
    
    attenuation_db = 20.0f * log10f(rms_after / rms_before);
}

// Funkcja główna do wywołania
void run_conversion_test(void)
{
    test_conversion_320_samples();
}
