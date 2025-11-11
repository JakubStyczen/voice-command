#ifndef VC_FILTER_H
#define VC_FILTER_H

#include "arm_math.h"
#include "voicecmd/vc_data_if.h"

// Parametry filtru Butterworth HPF 100 Hz, fs = 16 kHz
// (2nd order, bilinear transform)
#define VC_HPF_ORDER 2
#define VC_HPF_NUM_STAGES 1

// Współczynniki filtru w postaci float (normalizowane)
#define VC_HPF_B0  0.9726f
#define VC_HPF_B1 -1.9452f
#define VC_HPF_B2  0.9726f
#define VC_HPF_A1 -1.9444f
#define VC_HPF_A2  0.9459f

// Pre-emfaza (FIR) – współczynniki dla α = 0.97
#define VC_PREEMPH_ALPHA 0.97f
#define VC_PREEMPH_TAPS  2

#ifdef __cplusplus
extern "C" {
#endif

    // Funkcje konwersji int16_t <-> float32_t
    void vc_convert_int16_to_float32(const int16_t *src, float32_t *dst, uint32_t num_samples);
    void vc_convert_float32_to_int16(const float32_t *src, int16_t *dst, uint32_t num_samples);

    // Inicjalizacja i przetwarzanie filtru HPF
    void vc_hpf_init_f32(void);
    void vc_hpf_process_frame_f32(const vc_pcm_meta_t *pcm_meta, int16_t *input_data, float32_t *output_data);

    // Inicjalizacja i przetwarzanie pre-emfazy
    void vc_preemph_init_f32(void);
    void vc_preemph_process_f32(float32_t *data);

#ifdef __cplusplus
}
#endif

#endif // VC_FILTER_H
