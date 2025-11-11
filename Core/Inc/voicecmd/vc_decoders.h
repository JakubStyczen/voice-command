#ifndef VC_DECODERS_H
#define VC_DECODERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===== IMA-ADPCM decode: jeden blok -> PCM16 ===== */
#include <stdint.h>
#include "voicecmd/vc_data_if.h"
#include "arm_math.h"
#include "voicecmd/vc_encoders.h"
#include "g722_decoder.h"


/* Dekoder jednego bloku IMA (MS-IMA, mono, low-nibble-first).
 * block       -> wskaźnik na 4B nagłówka + spakowane nible (dokładnie jak zapisujesz)
 * spb         -> samples_per_block (np. 320)
 * out_pcm     -> int16_t[spb]
 * Zwraca liczbę wypisanych próbek (spb) lub 0 przy błędzie. */
uint16_t vc_ima_decode_block_mono(const uint8_t *block,
                                  uint16_t spb,
                                  int16_t *out_pcm);

void decompress_data(vc_pcm_meta_t *frm, int16_t *samples, vc_enc_cfg_t *cfg);

static G722_DEC_CTX *dec = NULL;

void g722_init_64k_dec(void);
void g722_deinit_dec(void);
int g722_decode_20ms_64k(const uint8_t *in160, int16_t *pcm320_out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VC_DECODERS_H */
