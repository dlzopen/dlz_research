from PIL import Image
import os
import struct
from export_utils import *

'''Run Script in this folder'''

class Dat():
    def __init__(self, width, height, total):
        self.width_ = width
        self.height_ = height
        self.total_ = total

    '''很多dat文件没有pal,只有索引。这些文件可以用这个函数，做统一处理'''
    def open(self, filename):
        self.buf = []
        self.filename = filename
        with open(get_path_in_dlz(filename), 'rb') as f:
            for i in range(self.total()):
                self.buf.append(list(f.read(self.width()*self.height())))
    
    def get_data(self, i):
        result = []
        for b in self.buf[i]:
            if b != 0:
                result.append(b)
            else:
                result.append(b)
        return result

    '''仅通过调色板索引输出的灰度图片，可以查看此图片描述的是什么'''
    def export(self):
        path = "../export/"+self.name()
        os.mkdir(path)
        img = Image.new("L", (self.width(), self.height()))
        for i in range(0, self.total()):
            img.putdata(self.get_data(i))
            # save new image
            img.save(f"{path}/{self.name()}_{i:02d}.png")
    
    def name(self):
        if hasattr(self, 'filename'):
            return os.path.splitext(self.filename)[0].title()
        else:
            return type(self).__name__
    
    def width(self):
        return self.width_

    def height(self):
        return self.height_

    def total(self):
        return self.total_

class Animation(Dat):
    def __init__(self, width, height, frame, count):
        self.width_ = width
        self.height_ = height
        self.frame_ = frame
        self.count_ = count
        self.total_ = frame * count

    def frame(self):
        return self.frame_
    def count(self):
        return self.count_

    '''仅通过调色板索引输出的灰度图片，可以查看此图片描述的是什么'''
    def export(self):
        path = "../export/"+self.name()
        os.mkdir(path)
        img = Image.new("L", (self.width(), self.height()))
        for i in range(0, self.total()):
            img.putdata(self.get_data(i))
            iframe = i % self.frame_
            icount = int(i / self.frame_)
            # save new image
            img.save(f"{path}/{self.name()}_{icount:02d}_{iframe:02d}.png")

class Place(Dat):
    def __init__(self):
        super().__init__(640, 480, 19)
        self.buf = []
        self.pal = []
        t = self.total()
        with open(get_path_in_dlz('place.dat'), 'rb') as f:
            for i in range(0, t):
                self.buf.append(list(f.read(self.width()*self.height())))
                self.pal.append(f.read(0x400))

    def get_data(self, i):
        result = []
        for b in self.buf[i]:
            t = tuple(self.pal[i][4*b:4*b+4])
            result.append((t[0], t[1], t[2], 255))
        return result

    @staticmethod
    def Export():
        di = Place()
        path = "../export/"+di.name()
        os.mkdir(path)
        img = Image.new("RGBA", (di.width(), di.height()))
        for i in range(0, di.total()):
            img.putdata(di.get_data(i))
            # save new image
            img.save(f"{path}/{di.name()}_{i:02d}.png")

        img.show()


class Face(Dat):
    def __init__(self):
        super().__init__(80, 100, 170)
        self.buf = []
        self.pal = []
        t = self.total()
        with open(get_path_in_dlz('face.dat'), 'rb') as f:
            for i in range(0, t):
                self.buf.append(list(f.read(self.width()*self.height())))

        for i in range(256):
            self.pal.append((i, i, i, 125))

    def get_data(self, i):
        result = []
        for b in self.buf[i]:
            result.append(self.pal[b])
            # result.append((b,b,b,255))
        return result

    '''通过人工截图<*_c.bmp>进行逆向调色板运算'''
    def update_palette(self, i):
        with open(f'..\\export\\{self.name()}_{i:02d}_c.bmp', 'rb') as f:
            f.seek(0x36)
            real = list(f.read(self.width()*self.height()*3))

            for ci, cd in enumerate(self.buf[i]):
                col = int(ci / self.width())
                row = ci % self.width()
                pi = (self.height() - col - 1) * self.width() + row
                palitem = real[pi*3:pi*3+3]
                palitem = [palitem[2], palitem[1], palitem[0], 255]
                self.pal[cd] = tuple(palitem)

    def report_palette(self):
        s = ''
        for i,j in enumerate(self.pal):
            if j[3] == 125:
                s += f' {i}'
        print(s)

        img = Image.new("RGBA", (16, 16))
        img.putdata(self.pal)
        # save new image
        img.save(f"export\\face_palette.bmp")
        img.show()

    @staticmethod
    def Export():
        di = Face()
        di.update_palette(1)
        di.update_palette(78)
        di.update_palette(79)
        di.update_palette(80)
        di.update_palette(81)
        di.update_palette(82)
        di.update_palette(83)
        di.update_palette(99)
        di.update_palette(100)
        di.update_palette(101)
        di.update_palette(103)
        di.update_palette(104)
        di.update_palette(105)
        di.update_palette(106)
        di.update_palette(107)
        di.update_palette(108)
        di.update_palette(125)
        di.report_palette()
        path = "../export/"+di.name()
        os.mkdir(path)
        img = Image.new("RGBA", (di.width(), di.height()))
        for i in range(0, di.total()):
            img.putdata(di.get_data(i))
            # save new image
            img.save(f"{path}/{di.name()}_{i:02d}.bmp")
            # img.show()

class BLK(Dat):
    def __init__(self):
        super().__init__(48, 48, 6860)
        self.buf = []
        self.index = []
        t = self.total()
        with open(get_path_in_dlz('blk.dat'), 'rb') as f:
            for i in range(0x40):
                offset = struct.unpack('I', f.read(4))[0]
                length = struct.unpack('I', f.read(4))[0]
                self.index.append([offset, length])

            for i in range(6860):
                self.buf.append(list(f.read(self.width()*self.height())))
    
    def export(self):
        path = "../export/"+self.name()
        img = Image.new("L", (self.width(), self.height()))
        i = 0
        j = 0
        for t in range(0, self.total()):
            if self.index[i][1] > 0:
                img.putdata(self.get_data(t))
                # save new image
                img.save(f"{path}/{i:02d}_{j:02d}.bmp")
            j = j + 1
            if (j * 48 * 48) > self.index[i][1]:
                j = 0
                i = i + 1

    @staticmethod
    def Export():
        di = BLK()
        di.export()


def export_army():
    an = Animation(48, 48, 19, 91)
    an.open('army.dat')
    an.export()

def export_blk():
    BLK.Export()

def export_face():
    Face.Export()

def export_fight():
    '''这是一个数据文件,参考export_fight.py dlz_fight.bt'''
    pass

def export_flm():
    an = Animation(48, 48, 16, 32)
    an.open('flm.dat')
    an.export()

def export_game():
    '''这是一个数据文件,参考export_game.py dlz_game.bt'''
    pass

def export_goods():
    dat = Dat(48, 48, 128)
    dat.open('goods.dat')
    dat.export()

def export_map():
    '''这是索引文件，和blk,pal共同合成一张地图'''
    '''这个应该不会写测试代码了'''
    pass

def export_material():
    dat = Dat(48, 48, 10)
    dat.open('material.dat')
    dat.export()
    Place.Export()

def export_mine():
    '''这个竟然忘了分析'''
    pass

def export_people():
    '''这是一个数据文件,参考export_people.py dlz_people.bt'''
    pass

def export_place():
    Place.Export()

def export_town():
    '''这是一个数据文件,参考export_town.py dlz_town.bt'''
    pass

def export_txt():
    '''这是一个数据文件,参考export_txt.py'''
    pass

def export_wave():
    '''这是一个数据文件,参考export_wave.py'''
    pass

if __name__ == "__main__":
    make_dir(DLZ_EXPORT)
    # export_army()
    export_blk()
    # export_flm()
    # export_goods()
    # export_material()
    # export_place()
    # export_town()
