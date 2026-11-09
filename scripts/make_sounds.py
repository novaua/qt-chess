"""Draws the app's generated sounds. Run from the repository root: python scripts\\make_sounds.py

sounds/lowtime.wav: two soft, falling notes when a clock goes low (LI-02).
sounds/challenge.wav: three rising notes when a Lichess challenge arrives (BL-10).
The format of the other sounds: 44.1 kHz, mono, 16 bit.
"""
import math
import struct
import wave

RATE = 44100


def note(frequency, seconds, volume):
    """A sine that starts softly and fades out, so it doesn't click."""
    count = int(RATE * seconds)
    samples = []
    for i in range(count):
        t = i / RATE
        attack = min(1.0, t / 0.004)
        decay = math.exp(-5.0 * t / seconds)
        tone = math.sin(2 * math.pi * frequency * t) + 0.25 * math.sin(4 * math.pi * frequency * t)
        samples.append(volume * attack * decay * tone / 1.25)
    return samples


def write(path, samples):
    with wave.open(path, 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(b''.join(struct.pack('<h', int(s * 32767)) for s in samples))


if __name__ == '__main__':
    pause = [0.0] * int(RATE * 0.04)
    write('ChessGui/ChessGui/sounds/lowtime.wav', note(988, 0.14, 0.5) + pause + note(740, 0.2, 0.5))
    write('ChessGui/ChessGui/sounds/challenge.wav',
          note(659, 0.12, 0.45) + pause + note(880, 0.12, 0.45) + pause + note(1319, 0.3, 0.45))
    print('ok')
