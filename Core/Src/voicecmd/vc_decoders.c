#include "voicecmd/vc_decoders.h"

void decompress_data(vc_pcm_meta_t *frm, int16_t *samples, vc_enc_cfg_t *cfg)
{
vc_stream_meta_t *m;
if (!frm || !cfg) return;


    switch (cfg->codec) {
    case VC_CODEC_PCM16:
        // TODO: load file encoded in PCM16 and fill samples array
        break;

    case VC_CODEC_IMA_ADPCM:
        uint8_t  adpcm[VC_IMA_MONO_BYTES_PER_FRAME];
        // TODO: load file encoded in IMA-ADPCM and fill adpcm array
        uint16_t result = vc_ima_decode_block_mono(adpcm, VC_MAX_FRAME_SAMPLES, samples);
        break;

    case VC_CODEC_G722:
        // -------------------------ALLLLERTT 
        // BEFORE USING 'g722_decode_20ms_64k' USE somewhere 'g722_init_64k_dec();' and after coding 'g722_deinit_dec();'
        //---------------------------------
        g722_init_64k_dec(); // maybe somewhere else
        int16_t g722_data[VC_G722_BYTES_PER_FRAME];
        // TODO dla wojtka: load file encoded in G722 and fill adpcm array
        int wrote_g722 = g722_decode_20ms_64k(g722_data, samples);
        if (wrote_g722 != 320) break; 
        g722_deinit_dec(); // maybe somewhere else
    }  
}

uint16_t vc_ima_decode_block_mono(const uint8_t *block,
                                  uint16_t spb,
                                  int16_t *out_pcm)
{
    if (!block || !out_pcm || spb == 0) return 0;

    /* Nagłówek 4B */
    int16_t predictor = (int16_t)( (uint16_t)block[0] | ((uint16_t)block[1] << 8) );
    int32_t index     = (int32_t)block[2];
    if (index < 0) index = 0; if (index > 88) index = 88;

    out_pcm[0] = predictor;

    const uint8_t *data = block + 4;
    uint16_t codes_needed = (spb > 0) ? (uint16_t)(spb - 1u) : 0u;

    for (uint16_t i = 0; i < codes_needed; i++) {
        /* low-nibble first w każdym bajcie */
        uint8_t byte = data[i >> 1];
        uint8_t code = ( (i & 1) == 0 ) ? (byte & 0x0F) : ((byte >> 4) & 0x0F);

        int32_t step = VC_IMA_STEP_TABLE[index];

        /* rekonstrukcja delta */
        int32_t delta = step >> 3;
        if (code & 1) delta += step >> 2;
        if (code & 2) delta += step >> 1;
        if (code & 4) delta += step;
        if (code & 8) delta = -delta;

        int32_t pred = (int32_t)predictor + delta;
        if (pred >  32767) pred =  32767;
        if (pred < -32768) pred = -32768;
        predictor = (int16_t)pred;

        out_pcm[i + 1] = predictor;

        /* adaptacja indeksu */
        index += VC_IMA_INDEX_TABLE[code & 7];
        if (index < 0) index = 0; if (index > 88) index = 88;
    }

    return spb;
}

void g722_init_64k_dec(void) {
    // 64 kb/s, wejście 16 kHz -> options = 0
    dec = g722_decoder_new(64000, 0);
}

void g722_deinit_dec(void) {
    if (dec) { g722_decoder_destroy(dec); dec = NULL; }
}

int g722_decode_20ms_64k(const uint8_t *in160, int16_t *pcm320_out) {
    // zwraca liczbę próbek; oczekuj 320
    return g722_decode(dec, in160, 160, pcm320_out);
}
