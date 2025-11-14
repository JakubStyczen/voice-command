#include "fatfs/app_fatfs.h"


int fatfs_init(){
    FATFS fs;
    FIL file;
    FRESULT res;

    // Zamontuj system plików na pendrive
    res = f_mount(&fs, "", 1);
    if(res != FR_OK) return -1;

    // Otwórz plik WAV do zapisu
    res = f_open(&file, "test.wav", FA_WRITE | FA_CREATE_ALWAYS);
    if(res != FR_OK) return -1;

    uint32_t sample_rate = 44100;   // 44.1 kHz
    uint16_t bits_per_sample = 16;  // 16-bit
    uint16_t channels = 1;          // mono

    // Przykładowe dane: 1 sekunda ciszy
    uint32_t num_samples = sample_rate * channels;
    uint32_t data_size = num_samples * (bits_per_sample / 8);

    // Zapisz nagłówek WAV
    write_wav_header(&file, sample_rate, bits_per_sample, channels, data_size);

    // Zapisz dane audio (cisza)
    uint8_t sample_data[2] = {0, 0}; // 16-bit zero
    for(uint32_t i = 0; i < num_samples; i++) {
        f_write(&file, sample_data, sizeof(sample_data), NULL);
    }

    // Zamknij plik
    f_close(&file);

    // Odmontuj pendrive
    f_mount(NULL, "", 1);

    return 0;
}

void write_wav_header(FIL *file, uint32_t sample_rate, uint16_t bits_per_sample, uint16_t channels, uint32_t data_size) {
    uint32_t chunk_size = 36 + data_size;
    uint16_t block_align = channels * bits_per_sample / 8;
    uint32_t byte_rate = sample_rate * block_align;

    f_lseek(file, 0);

    f_write(file, "RIFF", 4, NULL);
    f_write(file, &chunk_size, 4, NULL);
    f_write(file, "WAVE", 4, NULL);
    f_write(file, "fmt ", 4, NULL);

    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1; // PCM
    f_write(file, &subchunk1_size, 4, NULL);
    f_write(file, &audio_format, 2, NULL);
    f_write(file, &channels, 2, NULL);
    f_write(file, &sample_rate, 4, NULL);
    f_write(file, &byte_rate, 4, NULL);
    f_write(file, &block_align, 2, NULL);
    f_write(file, &bits_per_sample, 2, NULL);

    f_write(file, "data", 4, NULL);
    f_write(file, &data_size, 4, NULL);
}
