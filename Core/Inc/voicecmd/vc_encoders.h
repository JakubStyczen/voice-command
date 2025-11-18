#ifndef VC_ENCODERS_H
#define VC_ENCODERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "voicecmd/vc_data_if.h"
#include "arm_math.h"
#include "g722_encoder.h"
#include "g722_interface.h"

#define VC_MAX_FRAME_SAMPLES      320
#define VC_PCM16_BYTES_PER_FRAME  (VC_MAX_FRAME_SAMPLES * 2)
/* dla IMA-ADPCM mono: 4B nagłówka + ceil((N-1)/2) = 4 + 160 = 164 */
#define VC_IMA_MONO_BYTES_PER_FRAME  164
#define VC_G722_BYTES_PER_FRAME 160


typedef struct {
    vc_codec_id_t codec;          /* VC_CODEC_PCM16 / VC_CODEC_IMA_ADPCM / (opcjonalnie VC_CODEC_G711U) */
    uint16_t      samples_per_block; /* dla IMA-ADPCM: zwykle = 320 (Twoja ramka) ; 0 dla PCM/G.711 */
} vc_enc_cfg_t;

typedef struct {
    int16_t predictor;  /* diagnostyka; dekoder i tak startuje z nagłówka bloku */
    uint8_t index;      /* 0..88 */
} vc_ima_state_t;

void compress_data(vc_pcm_meta_t *frm, float32_t *samples, vc_enc_cfg_t *cfg);

void vc_meta_ima_adpcm_make(vc_stream_meta_t *m);

uint16_t vc_ima_block_bytes_mono(uint16_t spb);
void vc_meta_pcm16_make(vc_stream_meta_t *m);

/* ========== Tabele IMA ========== */
static const int16_t VC_IMA_STEP_TABLE[89] = {
     7,   8,   9,  10,  11,  12,  13,  14,  16,  17,
    19,  21,  23,  25,  28,  31,  34,  37,  41,  45,
    50,  55,  60,  66,  73,  80,  88,  97, 107, 118,
   130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
   337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
   876, 963,1059,1166,1282,1411,1552,1707,1878,2066,
  2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,
  5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,
 15289,16818,18500,20350,22385,24623,27086,29794,32767
};

static const int8_t VC_IMA_INDEX_TABLE[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

uint16_t vc_ima_encode_block_mono(int16_t *pcm,
                                               uint16_t spb,
                                               uint8_t *out_block,
                                               vc_ima_state_t *st /* może być NULL – wtedy lokalny reset */);



static G722_ENC_CTX *enc = NULL;

void g722_init_64k_enc(void);

void g722_deinit_enc(void);

int g722_encode_20ms_64k(const int16_t *pcm320, uint8_t *out160);

static G722EncInst *g_enc = NULL;
static G722DecInst *g_dec = NULL;

void g722_webrtc_init_64k(void);

int g722_webrtc_encode_20ms(const int16_t *pcm320, uint8_t *out160);

int g722_webrtc_decode_20ms(const uint8_t *in160, int16_t *pcm320_out);

void g722_webrtc_free(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VC_ENCODERS_H */
