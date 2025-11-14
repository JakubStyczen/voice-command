#ifndef __APP_FATFS_H
#define __APP_FATFS_H

#include "ff.h"
#include <stdint.h>
#include <string.h>

// Dane na pendrivie zmieniasz w FATFS/Target/ffconf.h
// zmiana formatu pendrive: f_mkfs (i inne w sekcji Volume Management and System Configuration)

int fatfs_init();
void write_wav_header(FIL *file, uint32_t sample_rate, uint16_t bits_per_sample, uint16_t channels, uint32_t data_size);

#endif
