#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "numpy",
#     "matplotlib",
#     "scipy",
#     "G722",
# ]
# ///
import matplotlib.pyplot as plt
import numpy as np
from G722 import G722  # PyPI: G722

SR_HZ = 16000
BITRATE = 64000  # zmień na 56000 albo 48000 jeśli chcesz
ALIGN = True  # wyrównanie opóźnienia (±32 próbki) przed SNR
MAX_LAG = 32
IGNORE_HEAD = 24  # pomiń kilka próbek (rozruch QMF) przy SNR
IGNORE_TAIL = 24

# === WKLEJ TU SWOJE 320 próbek int16 (oryginał przed kompresją) =================
orig_samples = [
    0,
    160,
    292,
    376,
    404,
    382,
    327,
    260,
    204,
    170,
    162,
    172,
    188,
    199,
    196,
    183,
    167,
    162,
    179,
    221,
    282,
    347,
    394,
    401,
    354,
    252,
    108,
    -54,
    -208,
    -326,
    -391,
    -401,
    -366,
    -305,
    -240,
    -190,
    -165,
    -164,
    -177,
    -193,
    -200,
    -193,
    -177,
    -164,
    -165,
    -190,
    -240,
    -305,
    -366,
    -401,
    -391,
    -326,
    -208,
    -54,
    108,
    252,
    354,
    401,
    394,
    347,
    282,
    221,
    179,
    162,
    167,
    183,
    196,
    199,
    188,
    172,
    162,
    170,
    204,
    260,
    327,
    382,
    404,
    376,
    292,
    160,
    0,
    -160,
    -292,
    -376,
    -404,
    -382,
    -327,
    -260,
    -204,
    -170,
    -162,
    -172,
    -188,
    -199,
    -196,
    -183,
    -167,
    -162,
    -179,
    -221,
    -282,
    -347,
    -394,
    -401,
    -354,
    -252,
    -108,
    54,
    208,
    326,
    391,
    401,
    366,
    305,
    240,
    190,
    165,
    164,
    177,
    193,
    200,
    193,
    177,
    164,
    165,
    190,
    240,
    305,
    366,
    401,
    391,
    326,
    208,
    54,
    -108,
    -252,
    -354,
    -401,
    -394,
    -347,
    -282,
    -221,
    -179,
    -162,
    -167,
    -183,
    -196,
    -199,
    -188,
    -172,
    -162,
    -170,
    -204,
    -260,
    -327,
    -382,
    -404,
    -376,
    -292,
    -160,
    0,
    160,
    292,
    376,
    404,
    382,
    327,
    260,
    204,
    170,
    162,
    172,
    188,
    199,
    196,
    183,
    167,
    162,
    179,
    221,
    282,
    347,
    394,
    401,
    354,
    252,
    108,
    -54,
    -208,
    -326,
    -391,
    -401,
    -366,
    -305,
    -240,
    -190,
    -165,
    -164,
    -177,
    -193,
    -200,
    -193,
    -177,
    -164,
    -165,
    -190,
    -240,
    -305,
    -366,
    -401,
    -391,
    -326,
    -208,
    -54,
    108,
    252,
    354,
    401,
    394,
    347,
    282,
    221,
    179,
    162,
    167,
    183,
    196,
    199,
    188,
    172,
    162,
    170,
    204,
    260,
    327,
    382,
    404,
    376,
    292,
    160,
    0,
    -160,
    -292,
    -376,
    -404,
    -382,
    -327,
    -260,
    -204,
    -170,
    -162,
    -172,
    -188,
    -199,
    -196,
    -183,
    -167,
    -162,
    -179,
    -221,
    -282,
    -347,
    -394,
    -401,
    -354,
    -252,
    -108,
    54,
    208,
    326,
    391,
    401,
    366,
    305,
    240,
    190,
    165,
    164,
    177,
    193,
    200,
    193,
    177,
    164,
    165,
    190,
    240,
    305,
    366,
    401,
    391,
    326,
    208,
    54,
    -108,
    -252,
    -354,
    -401,
    -394,
    -347,
    -282,
    -221,
    -179,
    -162,
    -167,
    -183,
    -196,
    -199,
    -188,
    -172,
    -162,
    -170,
    -204,
    -260,
    -327,
    -382,
    -404,
    -376,
    -292,
    -160,
    # ... 320 próbek int16 przed kompresją ...
]
# ================================================================================


def ensure_int16_320(arr):
    a = np.asarray(arr, dtype=np.int32)
    if a.size != 320:
        raise ValueError(f"Oczekuję dokładnie 320 próbek, mam {a.size}.")
    a = np.clip(a, -32768, 32767).astype(np.int16)
    return a


def align_by_xcorr(x, y, max_lag=32):
    best_lag, best_val = 0, -1e300
    for L in range(-max_lag, max_lag + 1):
        if L >= 0:
            xw = x[L:]
            yw = y[: len(xw)]
        else:
            yw = y[-L:]
            xw = x[: len(yw)]
        if len(xw) < 16:  # za krótko
            continue
        v = float(np.dot(xw.astype(np.int32), yw.astype(np.int32)))
        if v > best_val:
            best_val, best_lag = v, L
    if best_lag >= 0:
        x_al = x[best_lag:]
        y_al = y[: len(x_al)]
    else:
        y_al = y[-best_lag:]
        x_al = x[: len(y_al)]
    return x_al, y_al, best_lag


def snr_db(x, y, ig_head=0, ig_tail=0):
    N = min(len(x), len(y))
    a = ig_head
    b = N - ig_tail
    if b <= a:
        return float("nan")
    xo = x[a:b].astype(np.int32)
    yo = y[a:b].astype(np.int32)
    sig = np.sum(xo.astype(np.float64) ** 2)
    err = np.sum((xo - yo).astype(np.float64) ** 2)
    sig = max(sig, 1e-12)
    err = max(err, 1e-12)
    return 10.0 * np.log10(sig / err)


def mag_spectrum_dbfs(x_i16, fs):
    x = x_i16.astype(np.float32) / 32768.0
    w = np.hanning(x.size).astype(np.float32)
    X = np.fft.rfft(x * w)
    mag = np.abs(X) / (x.size / 2.0)
    mag[mag == 0] = 1e-12
    db = 20.0 * np.log10(mag)
    f = np.fft.rfftfreq(x.size, 1.0 / fs)
    return f, db


def main():
    # 1) Wejście
    x = ensure_int16_320(orig_samples)

    # 2) Encode→Decode w Pythonie (libg722 via PyPI)
    codec = G722(SR_HZ, BITRATE)  # API wg testów biblioteki
    payload = codec.encode(x)  # bytes G.722
    print(f"Payload: {len(payload)} B (bitrate={BITRATE})")
    y = np.asarray(codec.decode(payload), dtype=np.int16)  # lista -> int16

    # 3) (opcjonalnie) wyrównanie i SNR
    if ALIGN:
        x_al, y_al, lag = align_by_xcorr(x, y, MAX_LAG)
        print(
            f"[ALIGN] lag = {lag} próbek; porównuję {len(x_al)} próbek po przycięciu."
        )
    else:
        x_al, y_al = x, y

    val_snr = snr_db(x_al, y_al, IGNORE_HEAD, IGNORE_TAIL)
    print(f"SNR = {val_snr:.2f} dB  (ignore head={IGNORE_HEAD}, tail={IGNORE_TAIL})")

    # 4) FFT (przed)
    f1, db1 = mag_spectrum_dbfs(x_al, SR_HZ)
    plt.figure()
    plt.plot(f1, db1)
    plt.title("Widmo ORYGINAŁ (Hann, 320 próbek)")
    plt.xlabel("Częstotliwość [Hz]")
    plt.ylabel("Amplituda [dBFS]")
    plt.xlim(0, SR_HZ / 2)
    plt.grid(True, which="both", linestyle=":")
    plt.tight_layout()
    plt.savefig("fft_orig_g722lib.png", dpi=120)

    # 5) FFT (po)
    f2, db2 = mag_spectrum_dbfs(y_al, SR_HZ)
    plt.figure()
    plt.plot(f2, db2)
    plt.title("Widmo PO G.722 (Hann, 320 próbek)")
    plt.xlabel("Częstotliwość [Hz]")
    plt.ylabel("Amplituda [dBFS]")
    plt.xlim(0, SR_HZ / 2)
    plt.grid(True, which="both", linestyle=":")
    plt.tight_layout()
    plt.savefig("fft_dec_g722lib.png", dpi=120)

    print("Zapisano: fft_orig_g722lib.png, fft_dec_g722lib.png")
    # Podgląd kilku bajtów payloadu
    print("Payload[0:16] = ", payload[:16].hex(" "))
    print("Payload[0:16] = ", payload[-16:].hex(" "))


if __name__ == "__main__":
    main()
