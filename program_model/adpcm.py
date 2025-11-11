#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "numpy",
#     "matplotlib",
#     "scipy",
# ]
# ///
# ima_adpcm_mix_signal_demo_fft.py
# Generuje ramkę 320 próbek @16 kHz: 300 Hz (amp=300) + 900 Hz (amp=200) + 1500 Hz (amp=100),
# tnie do int16 (truncation), enkoduje do IMA-ADPCM (mono, MS-IMA), dekoduje,
# liczy SNR oraz rysuje widma (Hann, rFFT). Zapisuje też pliki wynikowe.

import struct

import matplotlib.pyplot as plt
import numpy as np

FS = 16000
N = 320


def test_generate_sine_f32(N, amplitude, freq_hz, fs=FS):
    t = np.arange(N, dtype=np.float32) / np.float32(fs)
    return (np.float32(amplitude) * np.sin(np.float32(2 * np.pi * freq_hz) * t)).astype(
        np.float32
    )


def test_compute_rms_f32(x):
    x = np.asarray(x, dtype=np.float32)
    return float(np.sqrt(np.mean(x * x)))


# --- Sygnał jak w Twoim teście (300 + 900 + 1500 Hz) ---
sig = test_generate_sine_f32(N, 300.0, 300.0)
sig += test_generate_sine_f32(N, 200.0, 900.0)
sig += test_generate_sine_f32(N, 100.0, 1500.0)

rms_input = test_compute_rms_f32(sig)
print(f"Sygnał wejściowy: (300 + 900 + 1500 Hz), RMS = {rms_input:.6f}")

# --- Konwersja do int16_t przez obcięcie (truncation) ---
pcm16 = np.trunc(sig).astype(np.int16, copy=False)
print("Pierwsze 24 próbki int16:", pcm16[:24].tolist())

# --- Tabele IMA-ADPCM (Microsoft IMA) ---
IMA_STEP_TABLE = np.array(
    [
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        16,
        17,
        19,
        21,
        23,
        25,
        28,
        31,
        34,
        37,
        41,
        45,
        50,
        55,
        60,
        66,
        73,
        80,
        88,
        97,
        107,
        118,
        130,
        143,
        157,
        173,
        190,
        209,
        230,
        253,
        279,
        307,
        337,
        371,
        408,
        449,
        494,
        544,
        598,
        658,
        724,
        796,
        876,
        963,
        1059,
        1166,
        1282,
        1411,
        1552,
        1707,
        1878,
        2066,
        2272,
        2499,
        2749,
        3024,
        3327,
        3660,
        4026,
        4428,
        4871,
        5358,
        5894,
        6484,
        7132,
        7845,
        8630,
        9493,
        10442,
        11487,
        12635,
        13899,
        15289,
        16818,
        18500,
        20350,
        22385,
        24623,
        27086,
        29794,
        32767,
    ],
    dtype=np.int32,
)

IMA_INDEX_TABLE = np.array([-1, -1, -1, -1, 2, 4, 6, 8], dtype=np.int32)


def ima_encode_block_mono(pcm: np.ndarray, start_index: int = 0) -> bytes:
    """Encode one IMA-ADPCM mono block (MS IMA, low-nibble first)."""
    assert pcm.dtype == np.int16 and pcm.ndim == 1 and pcm.size >= 1
    predictor = int(pcm[0])
    index = int(max(0, min(88, start_index)))
    out = bytearray(struct.pack("<hBB", predictor, index, 0))  # header
    low = True
    packed = 0

    for i in range(1, pcm.size):
        step = int(IMA_STEP_TABLE[index])
        diff = int(pcm[i]) - int(predictor)
        code = 0
        if diff < 0:
            code = 8
            diff = -diff

        t = step
        if diff >= t:
            code |= 4
            diff -= t
        t >>= 1
        if diff >= t:
            code |= 2
            diff -= t
        t >>= 1
        if diff >= t:
            code |= 1

        # reconstruct delta
        delta = step >> 3
        if code & 1:
            delta += step >> 2
        if code & 2:
            delta += step >> 1
        if code & 4:
            delta += step
        if code & 8:
            delta = -delta

        predictor = int(np.clip(predictor + delta, -32768, 32767))
        index = int(np.clip(index + IMA_INDEX_TABLE[code & 7], 0, 88))

        if low:
            packed = code & 0x0F
            low = False
        else:
            out.append((packed | ((code & 0x0F) << 4)) & 0xFF)
            low = True

    if not low:
        out.append(packed & 0xFF)

    return bytes(out)


def ima_decode_block_mono(block: bytes, samples_per_block: int):
    """Decode for quick validation (returns np.int16 of length samples_per_block)."""
    predictor, index, _ = struct.unpack_from("<hBB", block, 0)
    data = block[4:]
    # unpack low/high nibbles
    codes = []
    for b in data:
        codes.append(b & 0x0F)
        codes.append((b >> 4) & 0x0F)
    codes = codes[: max(0, samples_per_block - 1)]
    out = [np.int16(predictor)]
    for code in codes:
        step = int(IMA_STEP_TABLE[index])
        delta = step >> 3
        if code & 1:
            delta += step >> 2
        if code & 2:
            delta += step >> 1
        if code & 4:
            delta += step
        if code & 8:
            delta = -delta
        predictor = int(np.clip(predictor + delta, -32768, 32767))
        out.append(np.int16(predictor))
        index = int(np.clip(index + IMA_INDEX_TABLE[code & 7], 0, 88))
    return np.array(out, dtype=np.int16)


def hex_preview(b: bytes, max_len=64) -> str:
    s = " ".join(f"{x:02X}" for x in b[:max_len])
    if len(b) > max_len:
        s += f" ... (+{len(b) - max_len} B)"
    return s


# --- PCM bytes and ADPCM block ---
pcm_bytes = pcm16.tobytes()
adpcm_block = ima_encode_block_mono(pcm16, start_index=0)

print("\nRozmiary:")
print("PCM16:", len(pcm_bytes), "B (oczekiwane 640)")
print("IMA-ADPCM:", len(adpcm_block), "B (oczekiwane 164)")

print("\nADPCM nagłówek (4B):", " ".join(f"{x:02X}" for x in adpcm_block[:4]))
print("ADPCM payload (początek):", hex_preview(adpcm_block[4:], 64))

# --- Dekodowanie i SNR ---
dec = ima_decode_block_mono(adpcm_block, samples_per_block=N)
mse = float(np.mean((dec.astype(np.int32) - pcm16.astype(np.int32)) ** 2))
sig_pow = float(np.mean(pcm16.astype(np.int32) ** 2) + 1e-12)
snr_db = 10.0 * np.log10(sig_pow / max(mse, 1e-12))
print(f"\nDecoded MSE: {mse:.2f}, SNR ≈ {snr_db:.2f} dB")


# --- FFT (Hann, rFFT) dla oryginału i dekodowanego ---
def mag_spectrum_dbfs(x_i16, fs, use_hann=True):
    x = x_i16.astype(np.float32) / 32768.0
    N = x.size
    if use_hann:
        w = np.hanning(N).astype(np.float32)
        xw = x * w
    else:
        xw = x
    X = np.fft.rfft(xw)
    mag = np.abs(X) / (N / 2.0)  # proste skalowanie amplitudy
    mag[mag == 0] = 1e-12
    db = 20.0 * np.log10(mag)
    f = np.fft.rfftfreq(N, 1.0 / fs)
    return f, db


f_orig, db_orig = mag_spectrum_dbfs(pcm16, FS, use_hann=True)
f_dec, db_dec = mag_spectrum_dbfs(dec, FS, use_hann=True)

# wykres 1
plt.figure()
plt.plot(f_orig, db_orig)
plt.title("Widmo oryginału (PCM16) — Hann, N=320")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Amplituda [dBFS]")
plt.xlim(0, FS / 2)
plt.grid(True, which="both", linestyle=":")
plt.tight_layout()
plt.savefig("fft_orig.png", dpi=120)

# wykres 2
plt.figure()
plt.plot(f_dec, db_dec)
plt.title("Widmo po dekodowaniu (IMA-ADPCM) — Hann, N=320")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Amplituda [dBFS]")
plt.xlim(0, FS / 2)
plt.grid(True, which="both", linestyle=":")
plt.tight_layout()
plt.savefig("fft_decoded.png", dpi=120)

# --- Zapis binariów do inspekcji ---
with open("out_pcm16_mix.raw", "wb") as f:
    f.write(pcm_bytes)
with open("out_ima_adpcm_mix.bin", "wb") as f:
    f.write(adpcm_block)

print("\nZapisano pliki:")
print(" - out_pcm16_mix.raw (640 B)")
print(" - out_ima_adpcm_mix.bin (164 B)")
print(" - fft_orig.png, fft_decoded.png")
