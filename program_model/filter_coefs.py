#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "numpy",
#     "matplotlib",
#     "scipy",
# ]
# ///
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

fs = 16000
cutoff = 100
b, a = signal.butter(2, cutoff / (fs / 2), btype="highpass")
print("Original coefficients:")
print("b:", np.round(b, 7))
print("a:", np.round(a, 7))

# Method 1: Scale by a fixed factor (e.g., 32767 for maximum int16 range)
scale_factor = 16384  # Zmniejszamy, żeby uniknąć overflow
b_int16_method1 = (b * scale_factor).astype(np.int16)
a_int16_method1 = (a * scale_factor).astype(np.int16)

print("\nMethod 1 - Fixed scaling by 16384:")
print("b_int16:", b_int16_method1)
print("a_int16:", a_int16_method1)

# ==== GENEROWANIE SYGNAŁU TESTOWEGO ====
duration = 2.0  # sekundy
t = np.linspace(0, duration, int(fs * duration), False)

# Stworzenie sygnału testowego z różnymi częstotliwościami
freq1 = 50  # Poniżej cutoff (100 Hz) - powinno być wycięte
freq2 = 200  # Powyżej cutoff - powinno przejść
freq3 = 1000  # Wysoka częstotliwość - powinna przejść

# Komponenty sygnału
signal_50hz = 0.3 * np.sin(2 * np.pi * freq1 * t)
signal_200hz = 0.5 * np.sin(2 * np.pi * freq2 * t)
signal_1000hz = 0.2 * np.sin(2 * np.pi * freq3 * t)

# Dodanie szumu
noise = 0.1 * np.random.randn(len(t))

# Połączenie wszystkich komponentów
test_signal = signal_50hz + signal_200hz + signal_1000hz + noise
print(len(test_signal))
# Konwersja do int16 (jak w rzeczywistym systemie)
test_signal_int16 = (test_signal * 32767 * 0.8).astype(np.int16)
test_signal_float = test_signal_int16.astype(np.float64) / 32767

print("\nSygnał testowy:")
print(f"Długość: {duration} s")
print(f"Częstotliwości: {freq1} Hz, {freq2} Hz, {freq3} Hz + szum")
print(
    f"Zakres wartości int16: {np.min(test_signal_int16)} do {np.max(test_signal_int16)}"
)

# ==== FILTROWANIE ====
# Filtrowanie z oryginalnym filtrem float
filtered_float = signal.filtfilt(b, a, test_signal_float)

# Symulacja filtra int16 (przeskalowanego)
# Ważne: pierwszy współczynnik 'a' musi być = 1 (jest to standard IIR filters)
# Dlatego dzielimy wszystkie współczynniki przez a[0]
a_normalized = a / a[0]  # Normalizujemy oryginalny filtr
b_normalized = b / a[0]

# Skalujemy znormalizowane współczynniki
b_int16_normalized = (b_normalized * scale_factor).astype(np.int16)
a_int16_normalized = (a_normalized * scale_factor).astype(np.int16)

print("\nZnormalizowane współczynniki (a[0] = 1):")
print(f"b_normalized: {b_normalized}")
print(f"a_normalized: {a_normalized}")
print(f"b_int16_normalized: {b_int16_normalized}")
print(f"a_int16_normalized: {a_int16_normalized}")

# Konwersja z powrotem do float dla symulacji
b_scaled = b_int16_normalized.astype(np.float64) / scale_factor
a_scaled = a_int16_normalized.astype(np.float64) / scale_factor
# Upewniamy się, że a[0] = 1
a_scaled = a_scaled / a_scaled[0]

filtered_int16_sim = signal.filtfilt(b_scaled, a_scaled, test_signal_float)


# ==== ANALIZA FFT ====
def compute_fft(sig, fs):
    N = len(sig)
    fft_result = np.fft.fft(sig)
    freqs = np.fft.fftfreq(N, 1 / fs)
    # Tylko połowa (sygnał rzeczywisty)
    half_N = N // 2
    return freqs[:half_N], np.abs(fft_result[:half_N])


freqs, fft_original = compute_fft(test_signal_float, fs)
_, fft_filtered_float = compute_fft(filtered_float, fs)
_, fft_filtered_int16 = compute_fft(filtered_int16_sim, fs)

# ==== WYKRESY ====
plt.figure(figsize=(15, 12))

# Sygnał w dziedzinie czasu
plt.subplot(3, 2, 1)
plt.plot(t[:500], test_signal_float[:500], "b-", label="Oryginalny")
plt.plot(t[:500], filtered_float[:500], "r-", label="Filtrowany (float)")
plt.plot(
    t[:500], filtered_int16_sim[:500], "g--", label="Filtrowany (int16)", alpha=0.7
)
plt.title("Sygnał w dziedzinie czasu (pierwsze 500 próbek)")
plt.xlabel("Czas [s]")
plt.ylabel("Amplituda")
plt.legend()
plt.grid(True)

# FFT - skala liniowa
plt.subplot(3, 2, 2)
plt.plot(freqs, fft_original, "b-", label="Oryginalny", alpha=0.7)
plt.plot(freqs, fft_filtered_float, "r-", label="Filtrowany (float)")
plt.plot(freqs, fft_filtered_int16, "g--", label="Filtrowany (int16)", alpha=0.7)
plt.title("FFT - skala liniowa")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Amplituda")
plt.xlim(0, 2000)
plt.legend()
plt.grid(True)

# FFT - skala logarytmiczna
plt.subplot(3, 2, 3)
plt.semilogy(freqs, fft_original, "b-", label="Oryginalny", alpha=0.7)
plt.semilogy(freqs, fft_filtered_float, "r-", label="Filtrowany (float)")
plt.semilogy(freqs, fft_filtered_int16, "g--", label="Filtrowany (int16)", alpha=0.7)
plt.title("FFT - skala logarytmiczna")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Amplituda (log)")
plt.xlim(0, 2000)
plt.ylim(1e-3, None)
plt.axvline(x=cutoff, color="k", linestyle=":", label=f"Cutoff {cutoff} Hz")
plt.legend()
plt.grid(True)

# Zoom na niskie częstotliwości
plt.subplot(3, 2, 4)
plt.semilogy(freqs, fft_original, "b-", label="Oryginalny", alpha=0.7)
plt.semilogy(freqs, fft_filtered_float, "r-", label="Filtrowany (float)")
plt.semilogy(freqs, fft_filtered_int16, "g--", label="Filtrowany (int16)", alpha=0.7)
plt.title("FFT - zoom na niskie częstotliwości")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Amplituda (log)")
plt.xlim(0, 500)
plt.ylim(1e-4, None)
plt.axvline(x=cutoff, color="k", linestyle=":", label=f"Cutoff {cutoff} Hz")
plt.axvline(x=freq1, color="orange", linestyle=":", alpha=0.5, label=f"{freq1} Hz")
plt.axvline(x=freq2, color="purple", linestyle=":", alpha=0.5, label=f"{freq2} Hz")
plt.legend()
plt.grid(True)

# Charakterystyka częstotliwościowa filtra
plt.subplot(3, 2, 5)
w, h_float = signal.freqz(b, a, fs=fs)
w_int16, h_int16 = signal.freqz(b_scaled, a_scaled, fs=fs)
plt.plot(w, 20 * np.log10(abs(h_float)), "r-", label="Filtr (float)")
plt.plot(w_int16, 20 * np.log10(abs(h_int16)), "g--", label="Filtr (int16)", alpha=0.7)
plt.title("Charakterystyka częstotliwościowa filtra")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Magnitude [dB]")
plt.xlim(0, 1000)
plt.ylim(-80, 5)
plt.axvline(x=cutoff, color="k", linestyle=":", label=f"Cutoff {cutoff} Hz")
plt.axhline(y=-3, color="k", linestyle=":", alpha=0.5, label="-3 dB")
plt.legend()
plt.grid(True)

# Porównanie amplitud na kluczowych częstotliwościach
plt.subplot(3, 2, 6)
key_freqs = [freq1, freq2, freq3]
orig_amps = []
filt_float_amps = []
filt_int16_amps = []

for freq in key_freqs:
    idx = np.argmin(np.abs(freqs - freq))
    orig_amps.append(fft_original[idx])
    filt_float_amps.append(fft_filtered_float[idx])
    filt_int16_amps.append(fft_filtered_int16[idx])

x = np.arange(len(key_freqs))
width = 0.25

plt.bar(x - width, orig_amps, width, label="Oryginalny", alpha=0.7)
plt.bar(x, filt_float_amps, width, label="Filtrowany (float)", alpha=0.7)
plt.bar(x + width, filt_int16_amps, width, label="Filtrowany (int16)", alpha=0.7)

plt.title("Porównanie amplitud na kluczowych częstotliwościach")
plt.xlabel("Częstotliwość [Hz]")
plt.ylabel("Amplituda FFT")
plt.xticks(x, [f"{freq} Hz" for freq in key_freqs])
plt.legend()
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()

# ==== WYNIKI LICZBOWE ====
print("\n==== ANALIZA WYNIKÓW ====")
print(f"Cutoff filtra: {cutoff} Hz")
print("\nAmplitudy przed filtrowaniem:")
for i, freq in enumerate(key_freqs):
    print(f"  {freq} Hz: {orig_amps[i]:.4f}")

print("\nAmplitudy po filtrowaniu (float):")
for i, freq in enumerate(key_freqs):
    print(
        f"  {freq} Hz: {filt_float_amps[i]:.4f} (tłumienie: {20 * np.log10(filt_float_amps[i] / orig_amps[i]):.1f} dB)"
    )

print("\nAmplitudy po filtrowaniu (int16):")
for i, freq in enumerate(key_freqs):
    print(
        f"  {freq} Hz: {filt_int16_amps[i]:.4f} (tłumienie: {20 * np.log10(filt_int16_amps[i] / orig_amps[i]):.1f} dB)"
    )

print("\nRóżnica między filtrami float i int16:")
for i, freq in enumerate(key_freqs):
    diff_db = 20 * np.log10(filt_int16_amps[i] / filt_float_amps[i])
    print(f"  {freq} Hz: {diff_db:.2f} dB")
