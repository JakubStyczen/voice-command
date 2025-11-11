#include <stdio.h>
#include <math.h>
#include "arm_math.h"
#include "voicecmd/vc_agc.h"   // Twój plik AGC
#include "voicecmd/vc_data_if.h"

// ---------------------------------------------------
// Konfiguracja testu
#define FS_HZ          16000
#define FRAME_SAMPLES  320
#define FRAME_LEN_MS   (1000.0f * FRAME_SAMPLES / FS_HZ)
#define PI_F           3.14159265359f

// Pomocnicza funkcja do generowania sinusoidy
static void generate_sine(float32_t *buf, uint32_t N, float32_t amplitude, float32_t freq_hz)
{
    for (uint32_t i = 0; i < N; i++)
        buf[i] = amplitude * sinf(2.0f * PI_F * freq_hz * ((float)i / FS_HZ));
}

// Pomocnicza funkcja do pomiaru RMS (dla weryfikacji)
static float32_t compute_rms(const float32_t *buf, uint32_t N)
{
    float32_t sum = 0.0f;
    for (uint32_t i = 0; i < N; i++)
        sum += buf[i] * buf[i];
    return sqrtf(sum / N);
}

// ---------------------------------------------------
// Główny test AGC
void run_agc_test(void)
{
    vc_pcm_meta_t meta = {
        .frame_idx = 0,
        .sample_rate_hz = FS_HZ,
        .channels = 1,
        .len_samples = FRAME_SAMPLES
    };

    agc_f32_t agc;
    agc_f32_init(&agc, FS_HZ, FRAME_LEN_MS);

    float32_t frame[FRAME_SAMPLES];

    // --- Scenario A: zmienna amplituda (12 ramek)
    float32_t amps[] = {
        0.01f, 0.01f, 0.01f,   // cicho
        0.20f, 0.20f, 0.20f,   // głośno
        0.05f, 0.05f, 0.05f,   // średnio
        0.01f, 0.01f, 0.01f    // ponownie cicho
    };
    int num_frames = (int)(sizeof(amps) / sizeof(amps[0]));

    // Historie dla scenariusza A
    float32_t gains_next[sizeof(amps) / sizeof(amps[0])];
    float32_t rms_before_hist[sizeof(amps) / sizeof(amps[0])];
    float32_t rms_after_hist[sizeof(amps) / sizeof(amps[0])];

    for (int f = 0; f < num_frames; f++)
    {
        meta.frame_idx = (uint32_t)f;

        generate_sine(frame, FRAME_SAMPLES, amps[f], 1000.0f);

        float32_t gain_applied = agc.last_measured_gain;          // gain użyty NA TĘ ramkę
        float32_t rms_before   = compute_rms(frame, FRAME_SAMPLES);

        agc_f32_process(&meta, &agc, frame);                      // skaluje + liczy gain na następną

        float32_t rms_after = compute_rms(frame, FRAME_SAMPLES);
        float32_t gain_next = agc.last_measured_gain;             // gain na NASTĘPNĄ ramkę

        // zapisz historię
        gains_next[f]      = gain_next;
        rms_before_hist[f] = rms_before;
        rms_after_hist[f]  = rms_after;
    }

    // --- Scenario B: stała, zbyt duża amplituda (12 ramek)
    agc_f32_init(&agc, FS_HZ, FRAME_LEN_MS);                      // reset stanu AGC

    const int num_frames2 = 12;
    float32_t gains_next2[12];
    float32_t rms_before_hist2[12];
    float32_t rms_after_hist2[12];
    float32_t const_amp = 0.80f;

    for (int f = 0; f < num_frames2; ++f)
    {
        meta.frame_idx = (uint32_t)f;

        generate_sine(frame, FRAME_SAMPLES, const_amp, 1000.0f);

        float32_t gain_applied = agc.last_measured_gain;
        float32_t rms_before   = compute_rms(frame, FRAME_SAMPLES);

        agc_f32_process(&meta, &agc, frame);

        float32_t rms_after = compute_rms(frame, FRAME_SAMPLES);
        float32_t gain_next = agc.last_measured_gain;

        // zapisz historię
        gains_next2[f]      = gain_next;
        rms_before_hist2[f] = rms_before;
        rms_after_hist2[f]  = rms_after;
    }
}


