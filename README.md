# voice-command

TODO:
- sprawdzic jak odpalic projektu niekoniecznie z pliku main generowanego przez stm32 cube ide
- sprawdzic jakie pliki konfiguracyjne warto przeslac na githuba, a ktore olac
- wrzucic szkielet folderu projektu
- ustalic dokladny format danych

# Schemat przetwarzania

<img width="797" height="661" alt="image" src="https://github.com/user-attachments/assets/9574d229-d096-475c-98fe-a970d2b90ed6" />

```
MEMS mic (PDM 1-bit, ~MHz)
   │  (I2S generuje zegar, dane idą do DMA)
DMA/I2S RX ──► bufor PDM (ping-pong)
   │
PDM → PCM (decymacja + LPF do 16 kHz, 16-bit)
   │
Filtry: HPF 100 Hz (+ ewent. pre-emfaza)
   │
AGC/normalizacja (wyrównanie głośności)
   │
[opc.] VAD auto-trim (ucięcie ciszy na początku/końcu)
   │
Koder:
   ├─ PCM16  (brak kompresji)
   ├─ G.711  (8-bit, 2:1)
   └─ IMA-ADPCM (4-bit nibbles, 4:1; w blokach)
   │
Kontener WAV (fmt=PCM/G711/IMA + nagłówek RIFF)
   │
FatFs (f_open/f_write/f_close) → pendrive USB


Pendrive USB → FatFs
   │
Parser WAV (czyta „fmt ”, wie jaki kodek)
   │
Dekoder do PCM16:
   ├─ PCM:   tylko kopiowanie
   ├─ G.711: tablice µ-law/A-law → int16_t
   └─ IMA-ADPCM: z nagłówka bloku (predictor+index)
      odtwarzanie próbek z 4-bitowych nibbli
   │
[opc.] de-emfaza/limiter
   │
Bufor TX (ping-pong) → DMA → I2S TX
   │
Kodek CS43L22 (sterowany po I²C) → wyjście jack
```


# Struktura (Core, Inc i Src powinny byc generowane przez stm32cudeIDE)
```
/Core
│
├── Inc/                                # Pliki nagłówkowe (.h)
│   │
│   ├── voicecmd/                       # --- LOGIKA DSP I AUDIO (sprzętowo niezależna) ---
│   │    ├── voicecmd.h                 # Główne API wysokiego poziomu biblioteki (init, process, record)
│   │    ├── vad.h                      # Voice Activity Detection – energia + ZCR
│   │    ├── agc.h                      # Automatic Gain Control – adaptacyjne wzmocnienie
│   │    ├── filters.h                  # Filtry: preemfaza, HPF, BPF, de-esser (opcjonalnie)
│   │    ├── dsp_utils.h                # Pomocnicze funkcje DSP (okna, RMS, DC removal, ZCR)
│   │    ├── codec.h                    # Wspólny interfejs dla wszystkich kodeków
│   │    ├── codec_pcm16.h              # Kodek "zerowy" – surowy PCM16 (brak kompresji)
│   │    ├── codec_g711.h               # Kodek µ-law / A-law (kompresja logarytmiczna)
│   │    ├── codec_imaadpcm.h           # Kodek IMA ADPCM (kompresja 4:1)
│   │    ├── container.h                # Obsługa kontenerów WAV/VCMD (CRC, metadata, fs, codec_id)
│   │    └── ringbuf.h                  # Bufor cykliczny (dla danych PCM, próbek, ramek)
│   │
│   ├── port/                           # --- WARSTWA PORTUJĄCA (zależna od MCU/HAL) ---
│   │    ├── port_audio_in.h            # Interfejs wejścia audio (mikrofon MEMS, I2S+DMA)
│   │    ├── port_audio_out.h           # Interfejs wyjścia audio (I2S DAC, np. CS43L22)
│   │    ├── port_storage.h             # Interfejs zapisu/odczytu (FatFs, Flash)
│   │    ├── port_gpio_ui.h             # Obsługa UI (LED, przycisk, czujnik LIS3DSH tap)
│   │    └── port_config.h              # Konfiguracja pinów, zegarów i parametrów portów
│   │
│   ├── app/                            # --- WARSTWA APLIKACYJNA (logika programu) ---
│   │    ├── app_main.h                 # API głównej pętli, zarządzanie stanem (record/play)
│   │    └── app_cli.h                  # Interfejs konsolowy UART (debug, test, komendy)
│   │
│   └── hw_init.h                       # Deklaracje funkcji inicjalizacji sprzętu i HAL
│
└── Src/                                # Pliki implementacji (.c)
    │
    ├── voicecmd/                       # --- IMPLEMENTACJA LOGIKI AUDIO ---
    │    ├── voicecmd.c                 # Implementacja API wysokiego poziomu (init, start, stop, process)
    │    ├── vad.c                      # Detekcja mowy: energia, zero-cross rate
    │    ├── agc.c                      # AGC – automatyczna regulacja poziomu wejścia
    │    ├── filters.c                  # Filtry cyfrowe (FIR/IIR) – preemfaza, HPF, LPF
    │    ├── dsp_utils.c                # Funkcje DSP pomocnicze: RMS, okna, DC removal, interpolacja
    │    ├── codec_pcm16.c              # Implementacja prostego kodeka PCM16
    │    ├── codec_g711.c               # Implementacja kodeka G.711 (µ-law / A-law)
    │    ├── codec_imaadpcm.c           # Implementacja kodeka IMA ADPCM (4:1)
    │    ├── container.c                # Generowanie/zapis nagłówków WAV i VCMD + CRC
    │    └── ringbuf.c                  # Implementacja bufora cyklicznego dla ramek PCM
    │
    ├── port/                           # --- IMPLEMENTACJA PORTÓW SPRZĘTOWYCH ---
    │    ├── port_audio_in_stm32f4.c    # Obsługa mikrofonu PDM → PCM (I2S + DMA + PDM2PCM)
    │    ├── port_audio_out_stm32f4.c   # Obsługa wyjścia audio (I2S TX + DAC)
    │    ├── port_storage_usb_fatfs.c   # Zapis plików WAV/VCMD do pendrive (USB MSC + FatFs)
    │    ├── port_storage_flash.c       # Alternatywny zapis do pamięci Flash (logi, krótkie sample)
    │    └── port_gpio_ui.c             # Obsługa LEDów, przycisków, ewentualnie LIS3DSH (tap gesture)
    │
    ├── app/                            # --- GŁÓWNA LOGIKA PROGRAMU ---
    │    ├── app_main.c                 # Pętla główna, obsługa stanów (Idle, Record, Playback)
    │    └── app_cli.c                  # Parser poleceń UART/USB CLI (np. "rec", "play", "list")
    │
    ├── hw_init.c                       # Inicjalizacja systemu, zegarów, GPIO, DMA, I2S, USB, SDIO
    └── main.c                          # Punkt wejścia programu (CubeMX main) + wywołanie app_main()
 ```

 # Dodatkowe biblioteki

 ## CMSIS DSP
 https://community.st.com/t5/stm32-mcus/configuring-dsp-libraries-on-stm32cubeide/ta-p/49637
