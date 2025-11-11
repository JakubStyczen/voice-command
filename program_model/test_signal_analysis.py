#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "numpy",
#     "matplotlib",
#     "scipy",
# ]
# ///
"""
Skrypt analizujący sygnał testowy z projektu voice_commands_v2 i symulujący działanie filtrów.
Odtwarza dokładnie taki sam sygnał jak w funkcji generate_test_signal() z kodu C.
"""

import math

import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

# Stałe z projektu STM32
VC_FS_HZ = 16000  # częstotliwość próbkowania
VC_FRAME_SAMPLES = 320  # liczba próbek w ramce (20 ms)

# Współczynniki filtra HPF z kodu C (format Q15)
# Z pliku vc_filters.h:
VC_HPF_A1 = -31858
VC_HPF_A2 = 15498
VC_HPF_B0 = 15935
VC_HPF_B1 = -31870
VC_HPF_B2 = 15935

# Współczynniki pre-emphasis z kodu C (format Q15)
PREEMPH_COEFF_0 = 32767  # 1.0 w Q15
PREEMPH_COEFF_1 = -31884  # -0.9744 w Q15


def q15_to_float(q15_val):
    """Konwersja z formatu Q15 do float"""
    return q15_val / 32768.0


def float_to_q15(float_val):
    """Konwersja z float do formatu Q15"""
    return int(np.clip(float_val * 32768.0, -32768, 32767))


def generate_test_signal_c_like(num_samples, frame_counter=0):
    """
    Generuje dokładnie taki sam sygnał jak funkcja generate_test_signal() w kodzie C
    """
    samples = np.zeros(num_samples, dtype=np.int16)

    for i in range(num_samples):
        # Składowa niskoczęstotliwościowa (50 Hz) - powinna być ofiltrowana przez HPF
        low_freq = 5000.0 * math.sin(
            2.0 * math.pi * 50.0 * (frame_counter * num_samples + i) / VC_FS_HZ
        )

        # Składowa wysokofrequencyjna (1000 Hz) - powinna przejść przez HPF
        high_freq = 10000.0 * math.sin(
            2.0 * math.pi * 1000.0 * (frame_counter * num_samples + i) / VC_FS_HZ
        )

        # Mały szum DC - powinien być usunięty przez preemphasis
        dc_component = 1000.0

        samples[i] = int(low_freq + high_freq + dc_component)

    return samples


def calculate_rms(samples):
    """Oblicza RMS sygnału"""
    return np.sqrt(np.mean(samples.astype(np.float64) ** 2))


def apply_biquad_filter_q15(samples, b_coeffs, a_coeffs):
    """
    Symuluje filtr biquad w formacie Q15 jak w ARM CMSIS DSP
    b_coeffs = [b0, b1, b2]
    a_coeffs = [a1, a2] (bez a0, który jest zawsze 1)
    """
    # Konwersja współczynników Q15 do float
    b0 = q15_to_float(b_coeffs[0])
    b1 = q15_to_float(b_coeffs[1])
    b2 = q15_to_float(b_coeffs[2])
    a1 = q15_to_float(a_coeffs[0])
    a2 = q15_to_float(a_coeffs[1])

    # Stany filtru (x[n-1], x[n-2], y[n-1], y[n-2])
    x1, x2, y1, y2 = 0.0, 0.0, 0.0, 0.0

    output = np.zeros_like(samples, dtype=np.float64)

    for i, sample in enumerate(samples):
        x0 = sample / 32768.0  # Konwersja int16 do float

        # Równanie różnicowe filtru biquad
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2

        # Aktualizacja stanów
        x2, x1 = x1, x0
        y2, y1 = y1, y0

        # Konwersja z powrotem do int16 (z saturacją)
        output[i] = np.clip(y0 * 32768.0, -32768, 32767)

    return output.astype(np.int16)


def apply_preemphasis_q15(samples, coeffs):
    """
    Symuluje filtr FIR pre-emphasis w formacie Q15
    coeffs = [h0, h1] dla filtru y[n] = h0*x[n] + h1*x[n-1]
    """
    h0 = q15_to_float(coeffs[0])
    h1 = q15_to_float(coeffs[1])

    output = np.zeros_like(samples, dtype=np.float64)
    x1 = 0.0  # x[n-1]

    for i, sample in enumerate(samples):
        x0 = sample / 32768.0  # Konwersja int16 do float

        # Równanie FIR
        y0 = h0 * x0 + h1 * x1

        # Aktualizacja stanu
        x1 = x0

        # Konwersja z powrotem do int16 (z saturacją)
        output[i] = np.clip(y0 * 32768.0, -32768, 32767)

    return output.astype(np.int16)


def analyze_frequency_content(samples, fs, title=""):
    """Analizuje zawartość częstotliwościową sygnału"""
    N = len(samples)
    fft_result = np.fft.fft(samples.astype(np.float64))
    freqs = np.fft.fftfreq(N, 1 / fs)

    # Tylko dodatnie częstotliwości
    half_N = N // 2
    freqs_positive = freqs[:half_N]
    magnitude = np.abs(fft_result[:half_N])

    return freqs_positive, magnitude


def main():
    print("=== ANALIZA SYGNAŁU TESTOWEGO Z PROJEKTU voice_commands_v2 ===\n")

    # Generuj sygnał testowy (dokładnie jak w kodzie C)
    print("Generowanie sygnału testowego...")
    original_samples = generate_test_signal_c_like(VC_FRAME_SAMPLES, frame_counter=0)
    rms_original = calculate_rms(original_samples)

    print("Sygnał oryginalny:")
    print(f"  Liczba próbek: {len(original_samples)}")
    print(f"  RMS: {rms_original:.2f}")
    print(f"  Zakres: {np.min(original_samples)} do {np.max(original_samples)}")
    print(f"  Pierwsze 10 próbek: {original_samples[:10]}")
    print()

    # Test filtra HPF
    print("=== TEST FILTRU GÓRNOPRZEPUSTOWEGO (HPF) ===")
    print("Współczynniki filtru HPF (Q15):")
    print(f"  B0 = {VC_HPF_B0}, B1 = {VC_HPF_B1}, B2 = {VC_HPF_B2}")
    print(f"  A1 = {VC_HPF_A1}, A2 = {VC_HPF_A2}")

    print("Współczynniki filtru HPF (float):")
    print(
        f"  b0 = {q15_to_float(VC_HPF_B0):.6f}, b1 = {q15_to_float(VC_HPF_B1):.6f}, b2 = {q15_to_float(VC_HPF_B2):.6f}"
    )
    print(f"  a1 = {q15_to_float(VC_HPF_A1):.6f}, a2 = {q15_to_float(VC_HPF_A2):.6f}")
    print()

    # Zastosuj filtr HPF
    hpf_samples = apply_biquad_filter_q15(
        original_samples, [VC_HPF_B0, VC_HPF_B1, VC_HPF_B2], [VC_HPF_A1, VC_HPF_A2]
    )
    rms_hpf = calculate_rms(hpf_samples)

    print("Po filtrze HPF:")
    print(f"  RMS: {rms_hpf:.2f}")
    print(f"  Redukcja: {100.0 * (1.0 - rms_hpf / rms_original):.2f}%")
    print(f"  Zakres: {np.min(hpf_samples)} do {np.max(hpf_samples)}")
    print(f"  Pierwsze 10 próbek: {hpf_samples[:10]}")
    print()

    # Test pre-emphasis
    print("=== TEST PRE-EMPHASIS ===")
    print(f"Współczynniki pre-emphasis (Q15): [{PREEMPH_COEFF_0}, {PREEMPH_COEFF_1}]")
    print(
        f"Współczynniki pre-emphasis (float): [{q15_to_float(PREEMPH_COEFF_0):.6f}, {q15_to_float(PREEMPH_COEFF_1):.6f}]"
    )
    print()

    # Zastosuj pre-emphasis
    preemph_samples = apply_preemphasis_q15(
        original_samples, [PREEMPH_COEFF_0, PREEMPH_COEFF_1]
    )
    rms_preemph = calculate_rms(preemph_samples)

    print("Po pre-emphasis:")
    print(f"  RMS: {rms_preemph:.2f}")
    print(f"  Zmiana: {100.0 * (rms_preemph / rms_original - 1.0):.2f}%")
    print(f"  Zakres: {np.min(preemph_samples)} do {np.max(preemph_samples)}")
    print(f"  Pierwsze 10 próbek: {preemph_samples[:10]}")
    print()

    # Test kombinacji filtrów
    print("=== TEST HPF + PRE-EMPHASIS ===")
    # Najpierw HPF, potem pre-emphasis
    combined_samples = apply_biquad_filter_q15(
        original_samples, [VC_HPF_B0, VC_HPF_B1, VC_HPF_B2], [VC_HPF_A1, VC_HPF_A2]
    )
    combined_samples = apply_preemphasis_q15(
        combined_samples, [PREEMPH_COEFF_0, PREEMPH_COEFF_1]
    )
    rms_combined = calculate_rms(combined_samples)

    print("Po HPF + pre-emphasis:")
    print(f"  RMS: {rms_combined:.2f}")
    print(f"  Łączna zmiana: {100.0 * (rms_combined / rms_original - 1.0):.2f}%")
    print(f"  Zakres: {np.min(combined_samples)} do {np.max(combined_samples)}")
    print(f"  Pierwsze 10 próbek: {combined_samples[:10]}")
    print()

    # Analiza częstotliwościowa
    print("=== ANALIZA CZĘSTOTLIWOŚCIOWA ===")

    freqs_orig, mag_orig = analyze_frequency_content(original_samples, VC_FS_HZ)
    freqs_hpf, mag_hpf = analyze_frequency_content(hpf_samples, VC_FS_HZ)
    freqs_preemph, mag_preemph = analyze_frequency_content(preemph_samples, VC_FS_HZ)
    freqs_combined, mag_combined = analyze_frequency_content(combined_samples, VC_FS_HZ)

    # Znajdź amplitudy na częstotliwościach testowych
    test_freqs = [50, 1000]  # Hz
    print("Amplitudy na częstotliwościach testowych:")

    for freq in test_freqs:
        idx = np.argmin(np.abs(freqs_orig - freq))
        actual_freq = freqs_orig[idx]

        amp_orig = mag_orig[idx]
        amp_hpf = mag_hpf[idx]
        amp_preemph = mag_preemph[idx]
        amp_combined = mag_combined[idx]

        print(f"\n  {freq} Hz (rzeczywisty: {actual_freq:.1f} Hz):")
        print(f"    Oryginalny:     {amp_orig:.1f}")
        print(
            f"    Po HPF:         {amp_hpf:.1f} (tłumienie: {20 * np.log10(amp_hpf / amp_orig) if amp_orig > 0 else float('-inf'):.1f} dB)"
        )
        print(
            f"    Po preemph:     {amp_preemph:.1f} (zmiana: {20 * np.log10(amp_preemph / amp_orig) if amp_orig > 0 else float('-inf'):.1f} dB)"
        )
        print(
            f"    Po HPF+preemph: {amp_combined:.1f} (zmiana: {20 * np.log10(amp_combined / amp_orig) if amp_orig > 0 else float('-inf'):.1f} dB)"
        )

    # Wykresy
    print("\nGenerowanie wykresów...")

    plt.figure(figsize=(15, 12))

    # Sygnały w dziedzinie czasu
    plt.subplot(3, 2, 1)
    t = np.arange(VC_FRAME_SAMPLES) / VC_FS_HZ * 1000  # czas w ms
    plt.plot(t, original_samples, "b-", label="Oryginalny", alpha=0.8)
    plt.plot(t, hpf_samples, "r-", label="HPF", alpha=0.8)
    plt.plot(t, preemph_samples, "g-", label="Pre-emphasis", alpha=0.8)
    plt.plot(t, combined_samples, "m-", label="HPF+Preemph", alpha=0.8)
    plt.title("Sygnały w dziedzinie czasu")
    plt.xlabel("Czas [ms]")
    plt.ylabel("Amplituda")
    plt.legend()
    plt.grid(True, alpha=0.3)

    # Zoom na początek sygnału
    plt.subplot(3, 2, 2)
    zoom_samples = 50
    plt.plot(
        t[:zoom_samples],
        original_samples[:zoom_samples],
        "b-",
        label="Oryginalny",
        marker="o",
        markersize=3,
    )
    plt.plot(
        t[:zoom_samples],
        hpf_samples[:zoom_samples],
        "r-",
        label="HPF",
        marker="s",
        markersize=3,
    )
    plt.plot(
        t[:zoom_samples],
        preemph_samples[:zoom_samples],
        "g-",
        label="Pre-emphasis",
        marker="^",
        markersize=3,
    )
    plt.plot(
        t[:zoom_samples],
        combined_samples[:zoom_samples],
        "m-",
        label="HPF+Preemph",
        marker="d",
        markersize=3,
    )
    plt.title(f"Zoom na pierwsze {zoom_samples} próbek")
    plt.xlabel("Czas [ms]")
    plt.ylabel("Amplituda")
    plt.legend()
    plt.grid(True, alpha=0.3)

    # Spektra - skala liniowa
    plt.subplot(3, 2, 3)
    plt.plot(freqs_orig, mag_orig, "b-", label="Oryginalny", alpha=0.8)
    plt.plot(freqs_hpf, mag_hpf, "r-", label="HPF", alpha=0.8)
    plt.plot(freqs_preemph, mag_preemph, "g-", label="Pre-emphasis", alpha=0.8)
    plt.plot(freqs_combined, mag_combined, "m-", label="HPF+Preemph", alpha=0.8)
    plt.title("Spektra częstotliwościowe - skala liniowa")
    plt.xlabel("Częstotliwość [Hz]")
    plt.ylabel("Magnitude")
    plt.xlim(0, 2000)
    plt.axvline(x=50, color="orange", linestyle=":", alpha=0.7, label="50 Hz")
    plt.axvline(x=100, color="black", linestyle=":", alpha=0.7, label="Cutoff HPF")
    plt.axvline(x=1000, color="purple", linestyle=":", alpha=0.7, label="1000 Hz")
    plt.legend()
    plt.grid(True, alpha=0.3)

    # Spektra - skala logarytmiczna
    plt.subplot(3, 2, 4)
    plt.semilogy(freqs_orig, mag_orig + 1e-10, "b-", label="Oryginalny", alpha=0.8)
    plt.semilogy(freqs_hpf, mag_hpf + 1e-10, "r-", label="HPF", alpha=0.8)
    plt.semilogy(
        freqs_preemph, mag_preemph + 1e-10, "g-", label="Pre-emphasis", alpha=0.8
    )
    plt.semilogy(
        freqs_combined, mag_combined + 1e-10, "m-", label="HPF+Preemph", alpha=0.8
    )
    plt.title("Spektra częstotliwościowe - skala log")
    plt.xlabel("Częstotliwość [Hz]")
    plt.ylabel("Magnitude (log)")
    plt.xlim(0, 2000)
    plt.ylim(1e-1, None)
    plt.axvline(x=50, color="orange", linestyle=":", alpha=0.7, label="50 Hz")
    plt.axvline(x=100, color="black", linestyle=":", alpha=0.7, label="Cutoff HPF")
    plt.axvline(x=1000, color="purple", linestyle=":", alpha=0.7, label="1000 Hz")
    plt.legend()
    plt.grid(True, alpha=0.3)

    # Charakterystyka filtra HPF
    plt.subplot(3, 2, 5)
    # Stwórz filtr w formacie scipy
    b_float = [
        q15_to_float(VC_HPF_B0),
        q15_to_float(VC_HPF_B1),
        q15_to_float(VC_HPF_B2),
    ]
    a_float = [1.0, -q15_to_float(VC_HPF_A1), -q15_to_float(VC_HPF_A2)]

    w, h = signal.freqz(b_float, a_float, fs=VC_FS_HZ)
    plt.plot(w, 20 * np.log10(np.abs(h)), "r-", linewidth=2)
    plt.title("Charakterystyka częstotliwościowa HPF")
    plt.xlabel("Częstotliwość [Hz]")
    plt.ylabel("Magnitude [dB]")
    plt.xlim(0, 1000)
    plt.ylim(-60, 5)
    plt.axvline(x=50, color="orange", linestyle=":", alpha=0.7, label="50 Hz")
    plt.axvline(x=100, color="black", linestyle=":", alpha=0.7, label="Cutoff")
    plt.axvline(x=1000, color="purple", linestyle=":", alpha=0.7, label="1000 Hz")
    plt.axhline(y=-3, color="gray", linestyle="--", alpha=0.7, label="-3 dB")
    plt.legend()
    plt.grid(True, alpha=0.3)

    # Porównanie RMS
    plt.subplot(3, 2, 6)
    signals = ["Oryginalny", "HPF", "Pre-emphasis", "HPF+Preemph"]
    rms_values = [rms_original, rms_hpf, rms_preemph, rms_combined]
    colors = ["blue", "red", "green", "magenta"]

    bars = plt.bar(signals, rms_values, color=colors, alpha=0.7)
    plt.title("Porównanie wartości RMS")
    plt.ylabel("RMS")
    plt.xticks(rotation=45)
    plt.grid(True, alpha=0.3)

    # Dodaj wartości na słupkach
    for bar, rms_val in zip(bars, rms_values):
        plt.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + max(rms_values) * 0.01,
            f"{rms_val:.1f}",
            ha="center",
            va="bottom",
        )

    plt.tight_layout()
    plt.show()

    print("Analiza zakończona!")


if __name__ == "__main__":
    main()
