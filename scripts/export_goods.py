from PIL import Image
import os
import struct
from export_utils import *
from export_game import game

class Goods():
    def __init__(self):
        self.buf = []
        self.total = 128
        with open(get_path_in_dlz('goods.dat'), 'rb') as f:
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

if __name__ == "__main__":
    goods = Goods()
    make_dir(DLZ_EXPORT)
    goods.export()