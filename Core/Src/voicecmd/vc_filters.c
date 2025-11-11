#include "voicecmd/vc_filters.h"
#include "arm_math.h"

void vc_convert_int16_to_float32(const int16_t *src, float32_t *dst, uint32_t num_samples)
{
    if (!src || !dst || num_samples == 0)
        return;
        
    for (uint32_t i = 0; i < num_samples; i++) {
        dst[i] = (float32_t)src[i];
    }
}

// Konwersja float32_t -> int16_t (z saturacją)
void vc_convert_float32_to_int16(const float32_t *src, int16_t *dst, uint32_t num_samples)
{
    if (!src || !dst || num_samples == 0)
        return;
        
    for (uint32_t i = 0; i < num_samples; i++) {
        float32_t sample = src[i];
        dst[i] = (int16_t)sample;
    }
}

// ===================== HPF (2nd-order Butterworth 100 Hz, fs=16kHz) =====================

#define NUM_STAGES VC_HPF_NUM_STAGES

// Współczynniki filtru biquad: [b0, b1, b2, a1, a2]
static const float32_t biquad_coeffs[5 * NUM_STAGES] = {
    VC_HPF_B0, VC_HPF_B1, VC_HPF_B2, VC_HPF_A1, VC_HPF_A2
};

// Stan filtru (4*N)
static float32_t biquad_state[4 * NUM_STAGES];

// Instancja CMSIS-DSP
static arm_biquad_casd_df1_inst_f32 S_hpf;

// Inicjalizacja
void vc_hpf_init_f32(void)
{
    arm_biquad_cascade_df1_init_f32(&S_hpf, NUM_STAGES, biquad_coeffs, biquad_state);
}

// Przetwarzanie ramki HPF z metadanymi i oddzielnymi tablicami
void vc_hpf_process_frame_f32(const vc_pcm_meta_t *pcm_meta, int16_t *input_data, float32_t *output_data)
{
    if (!pcm_meta || !input_data || !output_data || pcm_meta->channels != 1)
        return;

    uint32_t N = pcm_meta->len_samples;
    if (N > VC_FRAME_SAMPLES) N = VC_FRAME_SAMPLES;
    
    float32_t temp_input_f32[VC_FRAME_SAMPLES];

    // Konwersja próbek z int16_t do float32_t
    vc_convert_int16_to_float32(input_data, temp_input_f32, N);

    // Przetwarzanie filtrem HPF: temp_input_f32 -> output_data
    arm_biquad_cascade_df1_f32(&S_hpf, temp_input_f32, output_data, N);
}


// ===================== Pre-Emfaza (FIR) =====================

// FIR: y[n] = x[n] - α*x[n−1]
// współczynniki: [1.0, -α]
static float32_t preemph_coeffs[VC_PREEMPH_TAPS] = {1.0f, -VC_PREEMPH_ALPHA};
static float32_t preemph_state[VC_PREEMPH_TAPS + VC_FRAME_SAMPLES - 1];
static arm_fir_instance_f32 preemph_inst;

// Inicjalizacja FIR pre-emfazy
void vc_preemph_init_f32(void)
{
    arm_fir_init_f32(&preemph_inst,
                     VC_PREEMPH_TAPS,
                     preemph_coeffs,
                     preemph_state,
                     VC_FRAME_SAMPLES);
}

// Przetwarzanie (in-place)
void vc_preemph_process_f32(float32_t *data)
{
    arm_fir_f32(&preemph_inst, data, data, VC_FRAME_SAMPLES);
}
