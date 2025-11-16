#!/usr/bin/env python
import sys
import wave
import struct
from math import sqrt


def load_wave(path):
    with wave.open(path, "rb") as wf:
        n_channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        framerate = wf.getframerate()
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    if sample_width != 2:
        raise SystemExit("Only 16-bit PCM WAV is supported")

    total_samples = n_frames * n_channels
    samples = struct.unpack("<" + "h" * total_samples, raw)

    # Convert to mono by RMS across channels
    mono = []
    for i in range(0, total_samples, n_channels):
        acc = 0.0
        for c in range(n_channels):
            v = samples[i + c] / 32768.0
            acc += v * v
        mono.append(sqrt(acc / n_channels))

    return mono, framerate


def detect_peaks(envelope, framerate, threshold=0.2, min_gap_ms=120):
    peaks = []
    min_gap = int(framerate * min_gap_ms / 1000.0)
    last_index = -min_gap

    for i, v in enumerate(envelope):
        if v < threshold:
            continue
        if i - last_index < min_gap:
            continue
        # Simple local maximum check
        left = envelope[i - 1] if i > 0 else 0.0
        right = envelope[i + 1] if i + 1 < len(envelope) else 0.0
        if v >= left and v >= right:
            peaks.append((i, v))
            last_index = i

    return peaks


def main():
    if len(sys.argv) < 2:
        print("Usage: analyze_audio_envelope.py <file.wav>")
        return

    path = sys.argv[1]
    envelope, framerate = load_wave(path)
    peaks = detect_peaks(envelope, framerate)

    if not peaks:
        print("No peaks above threshold; try lowering threshold or using a louder sample.")
        return

    # Compute basic stats: average peak value and intervals
    peak_values = [v for _, v in peaks]
    intervals = []
    for (i1, _), (i2, _) in zip(peaks, peaks[1:]):
        dt = (i2 - i1) / float(framerate)
        intervals.append(dt)

    print(f"Sample rate: {framerate} Hz")
    print(f"Detected peaks: {len(peaks)}")
    print(f"Peak amplitude range: min={min(peak_values):.3f}, max={max(peak_values):.3f}, avg={sum(peak_values)/len(peak_values):.3f}")
    if intervals:
        print(f"Peak interval (s): min={min(intervals):.3f}, max={max(intervals):.3f}, avg={sum(intervals)/len(intervals):.3f}")
    else:
        print("Only one peak detected; no interval statistics.")

    print("\nUse these stats to tune:")
    print("- strongMagnitude / strongJump based on typical peak amplitudes")
    print("- rhythmMinInterval / rhythmMaxInterval based on interval range")


if __name__ == "__main__":
    main()

