/*
 * vc_data_if.h — Wspólne typy danych dla biblioteki „voice-cmd” (STM32F407)
 *
 * Cel: jedno źródło prawdy dla formatu ramek/próbek i metadanych,
 *      używane przez moduły: A (tor audio I/O), B (DSP/kodeki), C (storage/formaty).
 *
 * Język: C99. Kompatybilne z C++ (extern "C").
 *
 * Konwencja: 16 kHz, mono, 16-bit PCM; ramka robocza = 20 ms = 320 próbek.
 *            ADPCM operuje blokami (np. 256 próbek/blok).
 *
 * (c) Zespół projektu „voice-cmd”
 */

#ifndef VC_DATA_IF_H
#define VC_DATA_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ====== Wersja interfejsu ====== */
#define VC_IF_VERSION_MAJOR 1
#define VC_IF_VERSION_MINOR 0

/* ====== Ustalenia globalne ====== */
#define VC_FS_HZ                 16000u     /* częstotliwość próbkowania */
#define VC_CHANNELS              1u         /* mono */
#define VC_PCM_BITS              16u
#define VC_PCM_BYTES_PER_SAMPLE  2u
#define VC_FRAME_MS              20u        /* ramka robocza */
#define VC_FRAME_SAMPLES         ((VC_FS_HZ * VC_FRAME_MS) / 1000u)  /* 320 */
#define VC_PCM_FRAME_BYTES       (VC_FRAME_SAMPLES * VC_PCM_BYTES_PER_SAMPLE) /* 640 B */
#define VC_FRAME_ALIGN_BYTES     4u         /* wyrównanie pamięci */
#define VC_MAX_PATH              64u        /* nazwy plików w CLI itp. */

/* ====== Makra pomocnicze ====== */
#ifndef VC_PACKED
#  if defined(__GNUC__)
#    define VC_PACKED __attribute__((packed))
#  else
#    define VC_PACKED
#  endif
#endif

//#ifndef VC_STATIC_ASSERT
//#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
//#    define VC_STATIC_ASSERT(cond, msg) _Static_assert(cond, #msg)
//#  else
//#    define VC_STATIC_ASSERT(cond, msg) typedef char static_assertion_##msg[(cond)?1:-1]
//#  endif
//#endif

/* ====== Kody zwrotne ====== */
typedef enum {
    VC_OK = 0,
    VC_E_AGAIN,        /* spróbuj ponownie (np. pusty bufor) */
    VC_E_FULL,         /* kolejka pełna */
    VC_E_EMPTY,        /* brak danych */
    VC_E_PARAM,        /* błędny parametr */
    VC_E_STATE,        /* niewłaściwy stan */
    VC_E_IO,           /* błąd I/O (USB/FatFs) */
    VC_E_CRC,          /* błąd sumy kontrolnej */
    VC_E_CODEC,        /* błąd kodeka/formatu */
} vc_status_t;

/* ====== Identyfikatory kodeków/formatów ====== */
typedef enum {
    VC_CODEC_PCM16   = 1,   /* Linear PCM 16-bit, WAV fmt=1 */
    VC_CODEC_G722   = 7,   /* G.711 µ-law, WAV fmt=7 */
    VC_CODEC_IMA_ADPCM = 0x11, /* IMA ADPCM, WAV fmt=0x11 (ADPCM Microsoft/IMA) */
} vc_codec_id_t;

/* ====== Ramka PCM dla przekazu A<->B i B<->A ====== */
/*
 * Uwaga: struct niesie tylko metadane + wskaźnik.
 * Bufor z próbkami jest zarządzany przez warstwę producenta (SPSC FIFO / ping-pong).
 */

/* Struktura tylko z metadanymi (bez danych) */
typedef struct VC_PACKED {
    uint32_t frame_idx;         /* ++ co ramkę (diagnostyka) */
    uint32_t sample_rate_hz;    /* = VC_FS_HZ */
    uint16_t channels;          /* = 1 */
    uint16_t len_samples;       /* zwykle = VC_FRAME_SAMPLES (320) */
} vc_pcm_meta_t;

//VC_STATIC_ASSERT(sizeof(int16_t) == 2, int16_must_be_2_bytes);

/* ====== Metadane strumienia do zapisu/odczytu ====== */
typedef struct VC_PACKED {
    vc_codec_id_t codec_id;     /* patrz vc_codec_id_t */
    uint32_t sample_rate_hz;    /* 16000 */
    uint16_t channels;          /* 1 */
    /* Dla PCM/G.711: */
    uint16_t bits_per_sample;   /* 16 (PCM) / 8 (G.711) lub 0 jeśli nie dotyczy */
    uint16_t block_align;       /* 2 (PCM16 mono) / 1 (G.711) / dla ADPCM: block_bytes */
    uint32_t avg_bytes_per_sec; /* 32000 (PCM16) / 16000 (G.711) / ~ (ADPCM zależnie od block) */
    /* Dla ADPCM: */
    uint16_t samples_per_block; /* np. 320 (ADPCM); 0 gdy nie dotyczy */
    uint16_t reserved;          /* wyrównanie */
} vc_stream_meta_t;

/* ====== Kawałki strumienia (uogólnienie) ====== */
typedef struct VC_PACKED {
    uint32_t seq;         /* ++ dla każdego chunku/bloku/ramki */
    uint32_t len_bytes;   /* rozmiar payload */
    const uint8_t* data;  /* wskaźnik na dane */
} vc_stream_chunk_t;

/* ====== G.711 ramka (przykładowo 20 ms @16 kHz = 320 B) ====== */
typedef vc_stream_chunk_t vc_g711_frame_t;

/* ====== IMA-ADPCM blok ====== */
/* Obliczanie rozmiaru bloku mono IMA-ADPCM:
 * block_bytes = 4 (nagłówek: predictor(int16), index(uint8), reserved(uint8))
 *             + ceil((samples_per_block - 1) / 2)
 */


/* ====== Nagłówek kontenera VCMD (lekki, little-endian) ====== */
#define VC_VCMD_MAGIC  0x56434D44u /* 'V''C''M''D' */
typedef struct VC_PACKED {
    uint32_t magic;           /* VC_VCMD_MAGIC */
    uint16_t version;         /* 0x0001 */
    uint16_t header_bytes;    /* rozmiar nagłówka (dla rozszerzeń) */
    uint32_t codec_id;        /* vc_codec_id_t */
    uint32_t sample_rate_hz;  /* 16000 */
    uint16_t channels;        /* 1 */
    uint16_t reserved0;
    uint32_t total_samples;   /* dla PCM/G711; dla ADPCM: total_samples po dekodowaniu */
    uint32_t total_blocks;    /* dla ADPCM/Speex (opcjonalnie) */
    uint32_t crc32;           /* CRC całego payloadu danych (0 = brak) */
    uint32_t flags;           /* bity opcji (np. indeks obecny) */
    /* ... (opcjonalnie) rozszerzenia lub indeks bloków */
} vc_vcmd_header_t;

/* ====== Flagi VCMD ====== */
enum {
    VC_VCMD_FLAG_HAS_INDEX = 1u << 0, /* załączony indeks bloków */
};

/* ====== Minimalny kontrakt kolejek SPSC ====== */
typedef struct {
    volatile uint32_t write_idx; /* producent */
    volatile uint32_t read_idx;  /* konsument */
    uint32_t capacity;           /* liczba elementów */
    void** slots;                /* tablica wskaźników na elementy (np. na ramki/chunki) */
} vc_spsc_fifo_t;

/* ====== Pomocnicze stałe i sprawdzenia ====== */
//VC_STATIC_ASSERT(VC_FRAME_SAMPLES == 320, frame_must_be_20ms_at_16k);
//VC_STATIC_ASSERT(VC_PCM_FRAME_BYTES == 640, pcm_frame_bytes_must_be_640);
//VC_STATIC_ASSERT(sizeof(vc_stream_meta_t) % 4 == 0, meta_struct_alignment);

/* ====== Dokumentacyjny przepływ ====== */
/*
A -> B (nagrywanie):
 - vc_pcm_frame_t (20 ms, 320 próbek, int16_t*)

B -> C (nagrywanie):
 - PCM:   vc_stream_meta_t{PCM16} + vc_stream_chunk_t (640 B/ramkę)
 - G.711: vc_stream_meta_t{G711U} + vc_g711_frame_t   (320 B/ramkę)
 - ADPCM: vc_stream_meta_t{IMA_ADPCM, samples_per_block=256, block_align=vc_ima_block_bytes_mono(256)}
          + vc_ima_adpcm_block_t (kolejne bloki ADPCM)

C -> B (odtwarzanie):
 - odpowiednio: chunki/ramki/bloki zgodnie z meta

B -> A (odtwarzanie):
 - vc_pcm_frame_t (20 ms, 320 próbek, int16_t*)
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VC_DATA_IF_H */
