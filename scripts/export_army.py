from shutil import get_unpack_formats
from PIL import Image
import os
import struct
from export_utils import *

is_transparent = False

class Army():
    def __init__(self):
        self.buf = []
        self.frame = 19
        self.count = 91
        self.pack = 2     # 0 : unpack; 1 : one unit pack; 2 : all packed
        self.HOV  = True  # only for one unit pack

        self.total = self.frame*self.count
        with open(get_path_in_dlz('army.dat'), 'rb') as f:
            for i in range(0, self.total):
                self.buf.append(list(f.read(48*48)))

    def get_pack_data(self, i_count):
        result = []
        p = get_palette(is_transparent)
        i_buf = self.frame * i_count
        if self.HOV:
            for r in range(48):
                for i_frame in range(self.frame):
                    i_buf = self.frame * i_count + i_frame
                    for b in self.buf[i_buf][48*r:48*(r+1)]:
                        result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        else:
            for i_frame in range(self.frame):
                for b in self.buf[i_buf]:
                    result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
                i_buf = i_buf + 1
        return result

    def get_unpack_data(self, i):
        result = []
        p = get_palette(is_transparent)
        for b in self.buf[i]:
            result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        return result

    def get_one_data(self):
        p = get_palette(is_transparent)
        result = []
        for i in range(self.count):
            for r in range(48):
                for i_frame in range(self.frame):
                    i_buf = self.frame * i + i_frame
                    for b in self.buf[i_buf][48*r:48*(r+1)]:
                        result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        return result

    def get_data(self, i):
        if self.pack != 0:
            return self.get_pack_data(i)
        else:
            return self.get_unpack_data(i)

    def export_one(self):
        path = DLZ_EXPORT + type(self).__name__ + '/'
        os.makedirs(path, exist_ok=True)
        img = Image.new("RGBA", (48*self.frame, 48*self.count))
        img.putdata(self.get_one_data())
        # save new image
        img.save(f"{path}/army.png")

    def export_pack(self):
        path = DLZ_EXPORT + type(self).__name__ + '/'
        os.makedirs(path, exist_ok=True)
        img = Image.new("RGBA", ((48 * self.frame) if self.HOV else 48, 48 if self.HOV else 48*self.frame))
        for i in range(0, self.count):
            img.putdata(self.get_data(i))
            # save new image
            H_V = "_H" if self.HOV else "_V"
            img.save(f"{path}/{i:002d}{H_V}.png")
            # img.show()

    def export_unpack(self):
        path = DLZ_EXPORT + type(self).__name__ + '/'
        os.makedirs(path, exist_ok=True)
        img = Image.new("RGBA", (48, 48))
        for i in range(0, self.total):
            img.putdata(self.get_data(i))
            iframe = i % self.frame
            icount = int(i / self.frame)
            # save new image
            img.save(f"{path}/{icount:02d}_{iframe:02d}.png")

    def export(self):
        if self.pack == 2:
            self.export_one()
        elif self.pack == 1:
            self.export_pack()
        else:
            self.export_unpack()

if __name__ == "__main__":
    army = Army()
    make_dir(DLZ_EXPORT)
    army.export()