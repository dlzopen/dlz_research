from PIL import Image
import os
import struct
from export_utils import *

class Mine():
    def __init__(self):
        self.buf = []
        self.total = 40
        with open(get_path_in_dlz('mine.dat'), 'rb') as f:
            for i in range(0, self.total):
                self.buf.append(list(f.read(48*48)))

    def get_data(self, i):
        result = []
        p = palette
        for b in self.buf[i]:
            if b != 0:
                result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
            else:
                result.append((0, 0, 0, 0))
        return result
    
    def export(self):
        path = DLZ_EXPORT + type(self).__name__.lower() + '/'
        make_dir(path)
        img = Image.new("RGBA", (48, 48))
        for i in range(0, self.total):
            img.putdata(self.get_data(i))
            # save new image
            img.save(f"{path}/{i:02d}.png")
            # img.show()

if __name__ == "__main__":
    mine = Mine()
    make_dir(DLZ_EXPORT)
    mine.export()