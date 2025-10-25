#ifndef VC_FILTER_H
#define VC_FILTER_H

#include "arm_math.h"
#include "vc_data_if.h"

// Definition for 2nd order Butterworth High-Pass Filter at 100 Hz
#define VC_HPF_ORDER 2
#define VC_HPF_A1 -31858
#define VC_HPF_A2 15498
#define VC_HPF_B0 15935
#define VC_HPF_B1 -31870
#define VC_HPF_B2 15935

#ifdef __cplusplus
extern "C"
{
#endif

    void init_hpf_q15(void);
    void process_frame_q15(vc_pcm_frame_t *frame);
    void preemph_init_q15(void);
    void preemph_process_q15(q15_t *data, uint32_t blockSize = VC_FRAME_SAMPLES);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_FILTER_H