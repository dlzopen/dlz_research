from PIL import Image
import os
import struct
from export_utils import *

class Face():
    def __init__(self):
        self.buf = []
        self.total = 170
        with open(get_path_in_dlz('face.dat'), 'rb') as f:
            for i in range(0, self.total):
                self.buf.append(list(f.read(80*100)))

    def get_data(self, i):
        result = []
        p = palette
        for b in self.buf[i]:
            result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        return result
    
    def export(self):
        path = DLZ_EXPORT + type(self).__name__ + '/'
        make_dir(path)
        img = Image.new("RGBA", (80, 100))
        for i in range(0, self.total):
            img.putdata(self.get_data(i))
            # save new image
            img.save(f"{path}/{i:002d}.png")
            # img.show()

if __name__ == "__main__":
    face = Face()
    make_dir(DLZ_EXPORT)
    face.export()