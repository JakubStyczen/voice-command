// /*
//  * vc_vad.c
//  *
//  *  Created on: Oct 31, 2025
//  *      Author: jaksty
//  */
// // VAD na CMSIS-DSP dla vc_pcm_frame_t
// // - RMS: arm_rms_f32 (po arm_q15_to_float)
// // - ZCR: szybki licznik na int16_t (zmiana bitu znaku)
// // - Histereza ON/OFF + "hangover" (utrzymanie mowy przez kilka ramek)
// // - Wycinanie ramek ciszy (kompaktowanie ramek mowy do wyjścia)

// #include <stdint.h>
// #include <stdbool.h>
// #include <string.h>
// #include "arm_math.h"   // CMSIS-DSP


// /* -------------------- Parametry i stan VAD -------------------- */
// typedef struct {
//     // Progi w dziedzinie RMS (float, 0..1). Dla AGC target~0.05 sprawdzają się:
//     float thr_rms_on;           // próg aktywacji mowy (np. 0.040f)
//     float thr_rms_off;          // próg wyłączenia mowy (np. 0.030f)
//     int   zcr_min;              // minimalna liczba przejść przez zero (np. 5)
//     int   zcr_max;              // maksymalna liczba (np. 80)
//     int   min_speech_frames;    // ile kolejnych ramek > prog, by potwierdzić start (np. 3)
//     int   hangover_frames;      // ile ramek utrzymać "mowę" po spadku poniżej progów (np. 4)
// } vad_params_t;

// typedef struct {
//     bool prev_speech;           // stan końcowy z poprzedniej ramki
//     int  consec_speech;         // licznik kolejnych ramek spełniających kryteria
//     int  hangover_left;         // ile ramek jeszcze trzymać mówienie po spadku
// } vad_state_t;

// typedef struct {
//     bool  raw_speech;           // wynik "surowy" (tylko progi)
//     bool  final_speech;         // wynik po histerezie/hangoverze
//     float rms;                  // RMS ramki (float 0..1)
//     int   zcr;                  // Zero Crossing Rate (liczba zmian znaku)
// } vad_metrics_t;

// /* -------------------- Narzędzia -------------------- */

// // Szybki ZCR na int16_t (PCM signed). Zmiana znaku => zmiana bitu 15.
// // Działa niezależnie od skali (AGC/normalizacja nie wpływa na ZCR).
// static int compute_zcr_i16(const int16_t *x, uint16_t N) {
//     if (N < 2) return 0;
//     int cnt = 0;
//     for (uint16_t i = 1; i < N; i++) {
//         // Jeśli bit znaku się różni, to przejście przez zero
//         if ( ((x[i] ^ x[i-1]) & 0x8000) != 0 ) cnt++;
//     }
//     return cnt;
// }

// // RMS na CMSIS-DSP: konwersja Q15(int16) -> float i arm_rms_f32.
// // Uwaga: arm_q15_to_float dzieli przez 32768.0f, więc zakres ~[-1, +1).
// static float compute_rms_q15_as_f32(const int16_t *x_q15, uint16_t N) {
//     // Bufor tymczasowy na float (możesz dać static, jeśli ramka ma stałe N)
//     float tmpf[512]; // wystarczy dla N<=512; dla 320 jest ok
//     arm_q15_to_float((const q15_t*)x_q15, tmpf, N);
//     float rms = 0.0f;
//     arm_rms_f32(tmpf, N, &rms);  // to jest już RMS (sqrt(mean(x^2)))
//     return rms;
// }

// /* -------------------- Detekcja jednej ramki -------------------- */
// static void vad_analyze_frame(const vc_pcm_frame_t *fr,
//                               const vad_params_t *p,
//                               const vad_state_t *st_in,
//                               vad_metrics_t *m_out)
// {
//     // 1) ZCR na int16_t (szybko i bez konwersji)
//     int zcr = compute_zcr_i16(fr->samples, fr->len_samples);

//     // 2) RMS na float z CMSIS-DSP
//     float rms = compute_rms_q15_as_f32(fr->samples, fr->len_samples);

//     // 3) Surowa decyzja na podstawie progów
//     bool raw = false;
//     if ( (rms > p->thr_rms_on) && (zcr >= p->zcr_min) && (zcr <= p->zcr_max) ) {
//         raw = true;
//     }
//     // Histereza ON/OFF na poziomie surowym (jeśli wcześniej było speech)
//     if (!raw && st_in->prev_speech && (rms > p->thr_rms_off)) {
//         raw = true;
//     }

//     // Zapisz metryki
//     m_out->rms = rms;
//     m_out->zcr = zcr;
//     m_out->raw_speech = raw;

//     // Uwaga: final_speech ustawiamy w warstwie smoothingu niżej
// }

// /* -------------------- Smoothing (min_speech + hangover) -------------------- */
// static bool vad_smooth_update(const vad_params_t *p,
//                               vad_state_t *st_io,
//                               bool raw_speech_now)
// {
//     // Licznik kolejnych ramek spełniających surowe kryteria
//     if (raw_speech_now) st_io->consec_speech++;
//     else                st_io->consec_speech = 0;

//     bool final_now = false;

//     if (st_io->prev_speech) {
//         // Już byliśmy w mowie
//         if (raw_speech_now) {
//             // Nadal mowa — resetuj hangover i potwierdź
//             st_io->hangover_left = p->hangover_frames;
//             final_now = true;
//         } else {
//             // Spadło poniżej progów — podtrzymuj jeszcze przez hangover
//             if (st_io->hangover_left > 0) {
//                 st_io->hangover_left--;
//                 final_now = true;
//             } else {
//                 final_now = false;
//             }
//         }
//     } else {
//         // Byliśmy w ciszy — wejdź w mowę dopiero po min_speech_frames
//         if (st_io->consec_speech >= p->min_speech_frames) {
//             st_io->prev_speech   = true;
//             st_io->hangover_left = p->hangover_frames;
//             final_now = true;
//         } else {
//             final_now = false;
//         }
//     }

//     // Aktualizacja stanu na kolejną ramkę
//     if (!final_now) {
//         // Jeśli ostatecznie cisza — reset flagi "prev_speech"
//         st_io->prev_speech = false;
//     } else {
//         st_io->prev_speech = true;
//     }

//     return final_now;
// }

// /* -------------------- API: analiza i cięcie strumienia ramek -------------------- */

// // 1) Analiza: wypełnia tablicę wyników (metrics + flaga final_speech)
// void vad_process_frames(const vc_pcm_frame_t *in_frames,
//                         uint32_t n_frames,
//                         const vad_params_t *p,
//                         vad_state_t *st,           // stan jest utrzymywany przez kolejne wywołania
//                         vad_metrics_t *out_metrics // [n_frames]
//                         )
// {
//     for (uint32_t i = 0; i < n_frames; i++) {
//         vad_metrics_t m;
//         vad_analyze_frame(&in_frames[i], p, st, &m);
//         bool final_flag = vad_smooth_update(p, st, m.raw_speech);
//         m.final_speech = final_flag;
//         out_metrics[i] = m;
//     }
// }

// // 2) Cięcie: tworzy wyjściową listę ramek mowy (kopiuje metadane i wskaźnik samples)
// //    Możesz potem po prostu przetwarzać/ zapisać tylko out_frames[0..*out_count-1]
// void vad_compact_speech_frames(const vc_pcm_frame_t *in_frames,
//                                uint32_t n_frames,
//                                const vad_metrics_t *metrics,
//                                vc_pcm_frame_t *out_frames, // dest
//                                uint32_t *out_count)
// {
//     uint32_t w = 0;
//     for (uint32_t i = 0; i < n_frames; i++) {
//         if (metrics[i].final_speech) {
//             out_frames[w++] = in_frames[i]; // płytka kopia; samples pokazuje na oryginalne dane
//         }
//     }
//     *out_count = w;
// }

// /* -------------------- Ustawienia domyślne i pomocnicze -------------------- */

// static inline void vad_params_default(vad_params_t *p) {
//     p->thr_rms_on        = 0.040f; // start (ok. sqrt(0.0016))
//     p->thr_rms_off       = 0.030f; // stop poniżej startu (histereza)
//     p->zcr_min           = 5;
//     p->zcr_max           = 80;
//     p->min_speech_frames = 3;      // ~60 ms potwierdzenia
//     p->hangover_frames   = 4;      // ~80 ms podtrzymania po spadku
// }

// static inline void vad_state_reset(vad_state_t *st) {
//     st->prev_speech   = false;
//     st->consec_speech = 0;
//     st->hangover_left = 0;
// }

// /* -------------------- (opcjonalnie) segmenty czasowe -------------------- */

// typedef struct {
//     uint32_t start_frame_idx; // inclusive
//     uint32_t end_frame_idx;   // inclusive
// } vad_segment_t;

// // Buduje listę segmentów (okresów mowy) na podstawie metrics.final_speech
// uint32_t vad_build_segments(const vc_pcm_frame_t *frames,
//                             const vad_metrics_t *metrics,
//                             uint32_t n_frames,
//                             vad_segment_t *segments,   // wyjście
//                             uint32_t max_segments)
// {
//     uint32_t count = 0;
//     bool in_seg = false;
//     uint32_t seg_start = 0;

//     for (uint32_t i = 0; i < n_frames; i++) {
//         bool s = metrics[i].final_speech;
//         if (!in_seg && s) {
//             in_seg = true;
//             seg_start = i;
//         } else if (in_seg && !s) {
//             in_seg = false;
//             if (count < max_segments) {
//                 segments[count].start_frame_idx = seg_start;
//                 segments[count].end_frame_idx   = i - 1;
//                 count++;
//             }
//         }
//     }
//     if (in_seg && count < max_segments) {
//         segments[count].start_frame_idx = seg_start;
//         segments[count].end_frame_idx   = n_frames - 1;
//         count++;
//     }
//     return count;
// }

// // Przeliczenie ramki na czas w ms: t_ms = frame_idx * len / fs * 1000
// static inline uint32_t frame_idx_to_ms(uint32_t frame_idx, uint16_t len_samples, uint32_t fs_hz) {
//     // zaokrąglenie w górę do ms
//     uint64_t num = (uint64_t)frame_idx * (uint64_t)len_samples * 1000ull;
//     return (uint32_t)(num / fs_hz);
// }


