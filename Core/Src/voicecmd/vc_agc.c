#include "voicecmd/vc_agc.h"

/**
 * @brief  Inicjalizacja AGC (float32)
 */
void agc_f32_init(agc_f32_t *agc, float fs, float frame_len_ms)
{
    agc->target_rms = 0.05f;  // docelowy RMS ~ -26 dBFS
    agc->current_gain = 1.0f;

    float frame_time = frame_len_ms / 1000.0f;
    float attack_time = 0.02f;    // 20 ms
    float release_time = 0.2f;    // 200 ms

    agc->alpha_attack  = expf(-frame_time / attack_time);
    agc->alpha_release = expf(-frame_time / release_time);
    agc->last_measured_gain = 1.0f;
}

/**
 * @brief  Aktualizuje gain na podstawie RMS ramki
 */
float32_t agc_f32_update_gain(agc_f32_t *agc, float32_t rms)
{
    float32_t desired_gain = agc->target_rms / (rms + 1e-6f);
    float32_t alpha = (desired_gain < agc->current_gain)
                        ? agc->alpha_attack
                        : agc->alpha_release;

    agc->current_gain =
        alpha * agc->current_gain + (1.0f - alpha) * desired_gain;

    return agc->current_gain;
}

/**
 * @brief  Przetwarza ramkę float32 na podstawie metadanych.
 */
void agc_f32_process(const vc_pcm_meta_t *meta, agc_f32_t *agc, float32_t *samples)
{
    if (!meta || !samples || meta->channels != 1)
        return;

    uint32_t N = meta->len_samples;

    arm_scale_f32(samples, agc->last_measured_gain, samples, N);

    float32_t rms;
    arm_rms_f32(samples, N, &rms);

    agc->last_measured_gain = agc_f32_update_gain(agc, rms);
}
