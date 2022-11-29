
import os
import struct
import wave

from export_utils import *

class Waves():
    def __init__(self) -> None:
        self.index = []
        self.arr = []
        with open(get_path_in_dlz('wave.dat'), 'rb') as f:
            for i in range(0x400):
                offset = struct.unpack('I', f.read(4))[0]
                length = struct.unpack('I', f.read(4))[0]
                self.index.append([offset, length])

            for i in range(0x400):
                self.arr.append(f.read(self.index[i][1]))

    def export(self):
        path = "../export/wave"
        os.makedirs(path, exist_ok=True)
        for i in range(0x400):
            if (self.index[i][1] == 0):
                continue
            f = wave.open(path+f"/{i:03d}.wav", "wb")
            f.setnchannels(2)
            f.setsampwidth(2)
            f.setframerate(44100/4)
            f.writeframes(self.arr[i])
            f.close()

waves = Waves()

if __name__ == "__main__":
    make_dir(DLZ_EXPORT)
    
    waves.export()