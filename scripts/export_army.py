from shutil import get_unpack_formats
from PIL import Image
import os
import struct
from export_utils import *

class Army():
    def __init__(self):
        self.buf = []
        self.frame = 19
        self.count = 91
        self.pack = False

        self.total = self.frame*self.count
        with open(get_path_in_dlz('army.dat'), 'rb') as f:
            for i in range(0, self.total):
                self.buf.append(list(f.read(48*48)))

    def get_pack_data(self, i_count):
        result = []
        p = palette
        i_buf = self.frame * i_count
        for i_frame in range(self.frame):
            for b in self.buf[i_buf]:
                result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
            i_buf = i_buf + 1
        return result

    def get_unpack_data(self, i):
        result = []
        p = palette
        for b in self.buf[i]:
            result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        return result


    def get_data(self, i):
        if self.pack:
            return self.get_pack_data(i)
        else:
            return self.get_unpack_data(i)

    def export_pack(self):
        path = DLZ_EXPORT + type(self).__name__ + '/'
        img = Image.new("RGBA", (48, 48*self.frame))
        for i in range(0, self.count):
            img.putdata(self.get_data(i))
            # save new image
            img.save(f"{path}/{i:002d}.png")
            # img.show()

    def export_unpack(self):
        path = DLZ_EXPORT + type(self).__name__ + '/'
        img = Image.new("RGBA", (48, 48))
        for i in range(0, self.total):
            img.putdata(self.get_data(i))
            iframe = i % self.frame
            icount = int(i / self.frame)
            # save new image
            img.save(f"{path}/{icount:02d}_{iframe:02d}.png")

    def export(self):
        if self.pack:
            self.export_pack()
        else:
            self.export_unpack()

if __name__ == "__main__":
    army = Army()
    make_dir(DLZ_EXPORT)
    army.export()