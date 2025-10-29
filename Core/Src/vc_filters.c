//#include "vc_filters.h"
//
//#define NUM_STAGES 1
//#define PREEMPH_TAPS 2
//
//// współczynniki filtru (Butterworth HPF 100 Hz, fs = 16 kHz)
//// Skalujemy do formatu Q15 (1.15), czyli mnożymy przez 32768
//
//static const q15_t biquad_coeffs[5 * NUM_STAGES] = {
//    VC_HPF_B0, VC_HPF_B1, VC_HPF_B2, -VC_HPF_A1, -VC_HPF_A2};
//
//static q15_t biquad_state[4 * NUM_STAGES];
//static arm_biquad_casd_df1_inst_q15 S;
//
//void init_hpf_q15(void)
//{
//    arm_biquad_cascade_df1_init_q15(&S, NUM_STAGES, biquad_coeffs, biquad_state, 1);
//}
//
//void process_frame_q15(vc_pcm_frame_t *frame)
//{
//    arm_biquad_cascade_df1_q15(&S, (q15_t *)frame->samples, (q15_t *)frame->samples, frame->len_samples, VC_FRAME_SAMPLES);
//}
//
//// Pre-emfaza (FIR) - współczynniki dla pre-emfazy 50 us przy fs = 16 kHz
//
//static q15_t preemph_coeffs[PREEMPH_TAPS] = {32767, -31884}; // {1.0, -0.9744} w Q15
//static q15_t preemph_state[PREEMPH_TAPS + PREEMPH_BLOCK - 1];
//
//static arm_fir_instance_q15 preemph_inst;
//
//void preemph_init_q15(void)
//{
//    arm_fir_init_q15(&preemph_inst, PREEMPH_TAPS,
//                     preemph_coeffs, preemph_state, VC_FRAME_SAMPLES);
//}
//
//void preemph_process_q15(q15_t *data, uint32_t blockSize = VC_FRAME_SAMPLES)
//{
//    arm_fir_q15(&preemph_inst, data, data, blockSize);
//}
