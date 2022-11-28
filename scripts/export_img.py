from PIL import Image
import os
import struct
from export_utils import *

class Img():
    def __init__(self, filename):
        self.buf = []
        self.total = 170
        self.filename = filename
        with open(get_path_in_dlz(filename), 'rb') as f:
            f.read(4)
            f.read(2)
            self.width = struct.unpack('H', f.read(2))[0]+1
            self.height = struct.unpack('H', f.read(2))[0]+1
            f.read(2)
            f.read(4)
            self.buf = list(f.read(self.width*self.height))
            self.pal = list(f.read(0x300))

    def get_data(self, i):
        result = []
        p = self.pal
        for b in self.buf:
            result.append((p[3*b + 0]*4, p[3*b + 1]*4, p[3*b + 2]*4, 255))
        return result
    
    # def export(self):
    #     path = DLZ_EXPORT + self.filename + '.png'

    #     img = Image.new("RGBA", (self.width, self.height))
    #     for i in range(0, 1):
    #         img.putdata(self.get_data(i))
    #         # save new image
    #         img.save(f"{path}")
    #         # img.show()

    def export(self):
        path = DLZ_EXPORT + self.filename + '.png'

        img = Image.new("P", (self.width, self.height))
        img.palette = get_image_palette(False)
        for i in range(0, 1):
            img.putdata(self.buf)
            # save new image
            img.save(f"{path}")
            # img.show()

if __name__ == "__main__":
    #img = Img('BG03.IMG')
    img = Img('MINE02.IMG')
    make_dir(DLZ_EXPORT)
    img.export()