"""
WAVE.DAT Audio Archive Export Script
=====================================

File Format Analysis: WAVE.DAT from Landmine War (地雷战, 1998)
----------------------------------------------------------------

WAVE.DAT is a custom audio archive containing raw PCM audio samples.
It uses a simple fixed-size index header followed by contiguous audio data.

STRUCTURE:
  [0x0000 - 0x1FFF]  Index Header (8192 bytes)
  [0x2000 - EOF]     Raw PCM audio data

INDEX HEADER:
  - 1024 entries (0x400), each 8 bytes
  - Each entry:
      offset  (uint32, little-endian): byte offset of audio data relative to
                                        the START of the data section (i.e.,
                                        relative to file offset 0x2000)
      length  (uint32, little-endian): length of audio data in bytes
  - Entries with length == 0 are unused/empty slots
  - 152 non-zero entries out of 1024, grouped in century ranges:
      000-009, 100-109, 200-209, ..., 900-999

AUDIO DATA:
  - Raw PCM, no RIFF/WAVE headers
  - Format confirmed from WAVEFORMATEX initialization in DLZ.exe (VA 0x7D0CF8):
      wFormatTag      = 1       (PCM)
      nChannels       = 1       (Mono)
      nSamplesPerSec  = 22050   (0x5622 Hz)
      nAvgBytesPerSec = 44100   (0xAC44)
      nBlockAlign     = 2       (1 sample * 2 bytes)
      wBitsPerSample  = 16      (0x10, signed 16-bit)
  - Playback via DirectSound (DSOUND.dll)

NOTES:
  - The previous version of this script incorrectly used stereo/11025 Hz.
    The correct parameters are mono/22050 Hz as proven by the WAVEFORMATEX
    structure in the executable. Duration is the same either way (since
    stereo@11025 and mono@22050 have the same byte rate of 44100 bytes/sec),
    but the pitch was wrong (one octave too low) and channel layout was wrong.
  - play_sound_effect() at VA 0x00405560 dispatches effects via a global
    table; it does not directly read WAVE.DAT. The actual audio loading
    happens in the DirectSound initialization code around VA 0x00023DA5.
"""

import os
import struct
import wave

from export_utils import *

# WAVE.DAT format constants
INDEX_ENTRIES = 0x400       # 1024 index slots
ENTRY_SIZE = 8              # 4 bytes offset + 4 bytes length per entry
HEADER_SIZE = INDEX_ENTRIES * ENTRY_SIZE  # 8192 bytes (0x2000)

# Audio format (from WAVEFORMATEX at VA 0x7D0CF8 in DLZ.exe)
SAMPLE_RATE = 22050         # Hz
BIT_DEPTH = 16              # bits per sample
NUM_CHANNELS = 1            # mono

class Waves():
    def __init__(self) -> None:
        self.index = []
        self.arr = []
        with open(get_path_in_dlz('wave.dat'), 'rb') as f:
            # Read 1024-entry index header (8192 bytes)
            for i in range(INDEX_ENTRIES):
                offset = struct.unpack('I', f.read(4))[0]
                length = struct.unpack('I', f.read(4))[0]
                self.index.append([offset, length])

            # Read raw PCM data for each entry sequentially
            # (file position is already at 0x2000, start of data section)
            for i in range(INDEX_ENTRIES):
                self.arr.append(f.read(self.index[i][1]))

    def export(self):
        path = os.path.join(DLZ_EXPORT, "wave")
        os.makedirs(path, exist_ok=True)
        exported = 0
        for i in range(INDEX_ENTRIES):
            if self.index[i][1] == 0:
                continue
            filepath = os.path.join(path, f"{i:03d}.wav")
            with wave.open(filepath, "wb") as wf:
                wf.setnchannels(NUM_CHANNELS)   # 1 = mono
                wf.setsampwidth(BIT_DEPTH // 8) # 2 bytes per sample
                wf.setframerate(SAMPLE_RATE)     # 22050 Hz
                wf.writeframes(self.arr[i])
            exported += 1
        print(f"Exported {exported} WAV files to {path}")

waves = Waves()

if __name__ == "__main__":
    make_dir(DLZ_EXPORT)

    waves.export()
