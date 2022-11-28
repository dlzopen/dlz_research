from PIL import Image
import os
import struct
from export_utils import *
from export_game import game

is_transparent = False

class Goods():
    def __init__(self):
        self.buf = []
        self.total = 128
        self.pack = True
        with open(get_path_in_dlz('goods.dat'), 'rb') as f:
            for i in range(0, self.total):
                self.buf.append(list(f.read(48*48)))

    def get_data(self, i):
        result = []
        p = get_palette(is_transparent)
        for b in self.buf[i]:
            result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        return result

    def export_pack(self):
        import math
        path = DLZ_EXPORT + type(self).__name__.lower() + '/'
        make_dir(path)
        col = 16
        row = int(math.ceil(self.total/col))
        img = Image.new("RGBA", (48 * col, 48 * row))
        p = get_palette(is_transparent)
        result = []
        for r in range(row):
            i_title_start = col * r
            for i in range(48):
                for c in range(col):
                    i_buf = i_title_start + c
                    if (i_buf >= self.total):
                        for t in range(48):
                            result.append((p[0], p[1], p[2], p[3]))
                    else:
                        for b in self.buf[i_buf][48*i:48*(i+1)]:
                            result.append((p[4*b + 0], p[4*b + 1], p[4*b + 2], p[4*b + 3]))
        img.putdata(result)
        img.save(f"{path}/goods.png")


    def export_unpack(self):
        path = DLZ_EXPORT + type(self).__name__.lower() + '/'
        make_dir(path)
        wupin = game.get_wupin()
        img = Image.new("RGBA", (48, 48))
        for i in range(0, self.total):
            img.putdata(self.get_data(i))
            wupin_name = ''
            # 一个图片被多个物品使用，我们只取第一个
            for wpi in wupin:
                if wpi.img_index == i and wpi.name() != '物　品':
                    wupin_name = wpi.name()
                    break
            # save new image
            if len(wupin_name) > 0:
                img.save(f"{path}/{i:03d}_{wupin_name}.png")
            else:
                img.save(f"{path}/{i:03d}.png")
            # img.show()

    def export(self):
        if self.pack:
            self.export_pack()
        else:
            self.export_unpack()

if __name__ == "__main__":
    goods = Goods()
    make_dir(DLZ_EXPORT)
    goods.export()