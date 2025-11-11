#include "voicecmd/vc_encoders.h"
#include "voicecmd/vc_decoders.h"
#include "voicecmd/vc_filters.h"

uint16_t vc_ima_block_bytes_mono(uint16_t spb)
{
    uint32_t nibbles = (spb > 0) ? (uint32_t)(spb - 1u) : 0u;
    uint32_t bytes_nibbles = (nibbles + 1u) / 2u;
    return (uint16_t)(4u + bytes_nibbles);
}


void compress_data(vc_pcm_meta_t *frm, float32_t *samples, vc_enc_cfg_t *cfg)
{
vc_stream_meta_t *m;
int16_t  to_file_raw[VC_MAX_FRAME_SAMPLES];
vc_convert_float32_to_int16(samples, to_file_raw, frm->len_samples);
if (!frm || !cfg) return;


    switch (cfg->codec) {
    case VC_CODEC_PCM16:
        vc_meta_pcm16_make(m);
        // TODO: save to file (data 'to_file_raw' with block size 640B, Metadata 'm')
        break;

    case VC_CODEC_IMA_ADPCM:
        vc_ima_state_t  ima_state = {0};
        uint8_t  adpcm[VC_IMA_MONO_BYTES_PER_FRAME];
        uint16_t wrote = vc_ima_encode_block_mono(to_file_raw, frm->len_samples, adpcm, &ima_state);
        vc_meta_ima_adpcm_make(m);
        /* got == 320; pcm_out to zdekodowany PCM16 (rekonstrukcja ADPCM) */

        // TODO: save to file dla wojtka
        
        // TODO: Moje pierdoly
        // int16_t pcm_out[320];
        // uint16_t got = vc_ima_decode_block_mono(adpcm, 320, pcm_out);
        break;

    case VC_CODEC_G722:
        // -------------------------ALLLLERTT 
        // BEFORE USING 'g722_encode_20ms_64k' USE somewhere 'g722_init_64k_enc();' and after coding 'g722_deinit_enc();'
        //---------------------------------
        g722_init_64k_enc(); // maybe somewhere else
        uint8_t payload[VC_G722_BYTES_PER_FRAME];                // 64 kb/s → 160 B / 20 ms
        int wrote_g722 = g722_encode_20ms_64k(to_file_raw, payload); // wrote_g722 == 0 SUCCESS?
        g722_deinit_enc(); // maybe somewhere else

        // TODO: save to file dla wojtka


        // TODO: Moje pierdoly
        // int16_t pcm_out_g722[320];
        // int kox = g722_decode_20ms_64k(payload, pcm_out_g722);
        // kox = 5;
        // if (wrote_g722 != VC_G722_BYTES_PER_FRAME) break; 
        break;
    }  
}

/* Metadane dla PCM16 (np. 16 kHz, mono). */
void vc_meta_pcm16_make(vc_stream_meta_t *m)
{
   if (!m) return;
   m->codec_id          = (vc_codec_id_t)0x0001;
   m->sample_rate_hz    = VC_FS_HZ;
   m->channels          = 1;
   m->bits_per_sample   = 16;                                /* PCM16 */
   m->block_align       = (uint16_t)(m->channels * 2u);         /* 2 dla mono 16-bit */
   m->avg_bytes_per_sec = m->sample_rate_hz * m->block_align;   /* np. 32000 */
   m->samples_per_block = 0;                                 /* nie dotyczy */
   m->reserved          = 0;
}


// void vc_meta_g722_make(vc_stream_meta_t *m)
// {
//    if (!m) return;
//    m->codec_id          = VC_CODEC_G722;
//    m->sample_rate_hz    = VC_FS_HZ;
//    m->channels          = 1;
//    m->bits_per_sample   = 16;                                /* PCM16 */
//    m->block_align       = (uint16_t)(m->channels * 2u);         /* 2 dla mono 16-bit */
//    m->avg_bytes_per_sec = m->sample_rate_hz * m->block_align;   /* np. 32000 */
//    m->samples_per_block = 0;                                 /* nie dotyczy */
//    m->reserved          = 0;
// }


/* ========== Metadane dla IMA-ADPCM (mono) ========== */
void vc_meta_ima_adpcm_make(vc_stream_meta_t *m)
{
   if (!m) return;
   m->codec_id          = VC_CODEC_IMA_ADPCM;
   m->sample_rate_hz    = VC_FS_HZ;
   m->channels          = 1;
   m->bits_per_sample   = 4;                             /* efektywnie ~4 bity/próbkę */
   m->samples_per_block = 320;
   m->block_align       = vc_ima_block_bytes_mono(m->samples_per_block);
   m->avg_bytes_per_sec = (m->samples_per_block > 0)
       ? (uint32_t)(((uint64_t)m->sample_rate_hz * m->block_align) / m->samples_per_block)
       : 0;
   m->reserved          = 0;
}






/* ========== Enkodowanie jednego bloku IMA-ADPCM (mono) ========== */
/* out_block musi mieć co najmniej vc_ima_block_bytes_mono(spb) bajtów.
* Zwraca liczbę zapisanych bajtów (= rozmiar bloku). */
uint16_t vc_ima_encode_block_mono(int16_t *pcm,
                                               uint16_t spb,
                                               uint8_t *out_block,
                                               vc_ima_state_t *st /* może być NULL – wtedy lokalny reset */)
{
   if (!pcm || !out_block || spb == 0) return 0;

   /* Inicjalizacja stanu na początek bloku:
      - predictor = pierwszy sample bloku
      - index: bierzemy z *st (ciągłość między blokami) lub 0 przy NULL */
   int16_t predictor = pcm[0];
   uint8_t index     = (st ? st->index : 0);
   if (index > 88) index = 88;

   /* Nagłówek bloku: predictor(int16 LE), index(uint8), reserved(uint8)=0 */
   out_block[0] = (uint8_t)(predictor & 0xFF);
   out_block[1] = (uint8_t)((uint16_t)predictor >> 8);
   out_block[2] = index;
   out_block[3] = 0;

   uint8_t *dst = out_block + 4;
   uint8_t packed = 0;
   int low_nibble = 1;

   /* Przechodzimy po próbkach 1..spb-1, każdą kodujemy do 4-bit nibbla.
      W Microsoft IMA kolejność to low-nibble najpierw. */
   for (uint16_t i = 1; i < spb; i++) {
       int step = VC_IMA_STEP_TABLE[index];

       int diff = (int)pcm[i] - (int)predictor;
       uint8_t code = 0;
       if (diff < 0) { code = 8; diff = -diff; }

       int temp = step;
       if (diff >= temp) { code |= 4; diff -= temp; }
       temp >>= 1;
       if (diff >= temp) { code |= 2; diff -= temp; }
       temp >>= 1;
       if (diff >= temp) { code |= 1; }

       /* Aktualizacja rekonstrukcji */
       int delta = step >> 3;
       if (code & 1) delta += step >> 2;
       if (code & 2) delta += step >> 1;
       if (code & 4) delta += step;
       if (code & 8) delta = -delta;

       int pred = (int)predictor + delta;
       if (pred >  32767) pred =  32767;
       if (pred < -32768) pred = -32768;
       predictor = (int16_t)pred;

       /* Aktualizacja indeksu */
       index += VC_IMA_INDEX_TABLE[code & 7];
       if ((int)index < 0) index = 0;
       if (index > 88) index = 88;

       /* Pakowanie do bajtów: low nibble -> pierwsza próbka po nagłówku */
       if (low_nibble) {
           packed = (code & 0x0F);
           low_nibble = 0;
       } else {
           *dst++ = (uint8_t)(packed | ((code & 0x0F) << 4));
           low_nibble = 1;
       }
   }
   /* Jeśli parzystość próbek zostawiła pół-bajt — dopisz ostatni bajt. */
   if (!low_nibble) {
       *dst++ = packed;
   }

   /* Zapisz stan dla kolejnego bloku (ciągłość), jeśli przekazano st */
   if (st) {
       st->predictor = predictor; /* nie jest używany jako nagłówek kolejnego bloku,
                                     bo kolejny blok i tak bierze predictor = pierwszy sample tamtego bloku,
                                     ale możesz go trzymać diagnostycznie */
       st->index = index;
   }

   return (uint16_t)(dst - out_block);
}


void g722_init_64k_enc(void) {
    // 64 kb/s, wejście 16 kHz -> options = 0
    enc = g722_encoder_new(64000, 0);
}


void g722_deinit_enc(void) {
    if (enc) { g722_encoder_destroy(enc); enc = NULL; }
}


int g722_encode_20ms_64k(const int16_t *pcm320, uint8_t *out160) {
    // zwraca liczbę bajtów; oczekuj 160
    return g722_encode(enc, pcm320, 320, out160);
}
