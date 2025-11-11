#ifndef VC_AGC_H
#define VC_AGC_H

#include "arm_math.h"
#include "voicecmd/vc_data_if.h"   // definicja vc_pcm_meta_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float32_t target_rms;         // docelowy RMS (np. 0.05)
    float32_t current_gain;       // aktualny gain
    float32_t alpha_attack;       // współczynnik attack
    float32_t alpha_release;      // współczynnik release
    float32_t last_measured_gain; // gain użyty w poprzedniej ramce
} agc_f32_t;

/**
 * Inicjalizacja struktury AGC.
 */
void agc_f32_init(agc_f32_t *agc, float fs, float frame_len_ms);

/**
 * Aktualizacja gainu na podstawie RMS.
 */
float32_t agc_f32_update_gain(agc_f32_t *agc, float32_t rms);

/**
 * Przetwarzanie ramki audio.
 *
 * @param meta     metadane ramki (długość, fs, kanały)
 * @param agc      wskaźnik na strukturę AGC
 * @param samples  wskaźnik na dane audio float32_t
 */
void agc_f32_process(const vc_pcm_meta_t *meta, agc_f32_t *agc, float32_t *samples);

#ifdef __cplusplus
}
#endif

#endif /* VC_AGC_H */



