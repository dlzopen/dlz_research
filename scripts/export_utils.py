
import os

DLZ_PATH = "../DLZFULL/"
DLZ_EXPORT = "../export/"

TILE_WIDTH = 48
TILE_HEIGHT = 48

def make_dir(path):
    os.makedirs(path, exist_ok=True)

def get_path_in_dlz(filename):
    return DLZ_PATH + filename

def save_img(img, filename):
    print(f"saving \'{filename}\'...")
    img.save(filename)

palette = [
    0x8c,0x8c,0x78,0xFF,0xf8,0xc0,0xb0,0xFF,0xfc,0xb4,0xac,0xFF,0xfc,0xbc,0x98,0xFF,
    0xf8,0xbc,0x98,0xFF,0xfc,0xb4,0x94,0xFF,0xfc,0xac,0x8c,0xFF,0xf0,0xa4,0x7c,0xFF,
    0xf4,0x9c,0x7c,0xFF,0xfc,0xa8,0x74,0xFF,0xf0,0xa8,0x78,0xFF,0xfc,0xb4,0x78,0xFF,
    0xfc,0xbc,0xb0,0xFF,0xf0,0xac,0x94,0xFF,0xec,0xc4,0x80,0xFF,0xfc,0xcc,0x94,0xFF,
    0xdc,0xa8,0x90,0xFF,0xd4,0x7c,0x28,0xFF,0xd8,0x80,0x48,0xFF,0xe4,0x84,0x44,0xFF,
    0xe8,0x8c,0x44,0xFF,0xf4,0x9c,0x58,0xFF,0xf8,0x98,0x70,0xFF,0xe8,0x88,0x60,0xFF,
    0xe4,0x80,0x60,0xFF,0xe4,0x78,0x54,0xFF,0xd4,0x74,0x50,0xFF,0xfc,0x78,0x60,0xFF,
    0xfc,0x60,0x60,0xFF,0xe0,0x54,0x34,0xFF,0xb8,0x4c,0x40,0xFF,0xac,0x44,0x38,0xFF,
    0x88,0x68,0x5c,0xFF,0x74,0x58,0x4c,0xFF,0xd8,0x70,0x38,0xFF,0xc8,0x64,0x2c,0xFF,
    0xac,0x70,0x48,0xFF,0xac,0x64,0x50,0xFF,0xc8,0x70,0x50,0xFF,0xd0,0x78,0x58,0xFF,
    0xc0,0x7c,0x50,0xFF,0xc4,0x80,0x20,0xFF,0xbc,0x74,0x34,0xFF,0xb8,0x68,0x10,0xFF,
    0xb4,0x60,0x1c,0xFF,0xac,0x5c,0x20,0xFF,0xa8,0x50,0x14,0xFF,0xa0,0x4c,0x14,0xFF,
    0x8c,0x3c,0x0c,0xFF,0xa4,0x44,0x20,0xFF,0x90,0x3c,0x2c,0xFF,0x7c,0x30,0x00,0xFF,
    0x70,0x20,0x00,0xFF,0x68,0x14,0x10,0xFF,0x60,0x24,0x04,0xFF,0x90,0x4c,0x20,0xFF,
    0x84,0x40,0x14,0xFF,0x78,0x38,0x04,0xFF,0x68,0x34,0x18,0xFF,0x60,0x2c,0x00,0xFF,
    0x50,0x28,0x0c,0xFF,0x40,0x1c,0x04,0xFF,0x30,0x14,0x00,0xFF,0x18,0x08,0x00,0xFF,
    0x50,0x7c,0x94,0xFF,0x58,0x88,0xa0,0xFF,0x60,0x98,0xac,0xFF,0x6c,0xa8,0xb8,0xFF,
    0x78,0xb8,0xc8,0xFF,0x80,0xc8,0xd4,0xFF,0x8c,0xd8,0xe0,0xFF,0x98,0xe8,0xec,0xFF,
    0xa4,0xfc,0xfc,0xFF,0x6c,0x7c,0x94,0xFF,0x74,0x88,0xa8,0xFF,0x80,0x94,0xc0,0xFF,
    0x88,0x9c,0xd4,0xFF,0x90,0xa4,0xec,0xFF,0x9c,0xb4,0xf4,0xFF,0xa8,0xc4,0xfc,0xFF,
    0x24,0x08,0x00,0xFF,0x24,0x0c,0x00,0xFF,0x28,0x14,0x04,0xFF,0x2c,0x1c,0x08,0xFF,
    0x30,0x24,0x10,0xFF,0x34,0x28,0x18,0xFF,0x38,0x30,0x20,0xFF,0x3c,0x38,0x28,0xFF,
    0x44,0x3c,0x2c,0xFF,0x4c,0x44,0x34,0xFF,0x58,0x50,0x3c,0xFF,0x64,0x58,0x44,0xFF,
    0x74,0x64,0x4c,0xFF,0x84,0x74,0x58,0xFF,0x94,0x84,0x64,0xFF,0xa4,0x94,0x70,0xFF,
    0x84,0x9c,0xa8,0xFF,0x7c,0x9c,0xa4,0xFF,0x74,0x98,0xa0,0xFF,0x90,0xa4,0xb0,0xFF,
    0x98,0xa8,0xb4,0xFF,0xa4,0xb0,0xbc,0xFF,0xb0,0xb4,0xc0,0xFF,0xbc,0xc0,0xc8,0xFF,
    0xc0,0xc4,0xcc,0xFF,0xc8,0xcc,0xd4,0xFF,0xd0,0xd4,0xd8,0xFF,0xd8,0xdc,0xe0,0xFF,
    0xe0,0xe4,0xe8,0xFF,0xe8,0xec,0xec,0xFF,0xf0,0xf0,0xf4,0xFF,0xfc,0xfc,0xfc,0xFF,
    0x54,0x80,0x98,0xFF,0x4c,0x78,0x98,0xFF,0x78,0x80,0x98,0xFF,0x50,0x74,0x84,0xFF,
    0x00,0x84,0xd4,0xFF,0x7c,0xbc,0xdc,0xFF,0x68,0x98,0xb0,0xFF,0x5c,0x84,0xac,0xFF,
    0x4c,0x80,0xa0,0xFF,0x4c,0xac,0x98,0xFF,0x54,0x80,0xa8,0xFF,0x54,0x64,0x1c,0xFF,
    0x70,0x70,0x28,0xFF,0x78,0x7c,0x28,0xFF,0x78,0x9c,0xc4,0xFF,0x80,0x98,0xbc,0xFF,
    0xf0,0xb8,0x50,0xFF,0xd8,0xa4,0x44,0xFF,0xd0,0xd0,0x7c,0xFF,0xc0,0xb4,0x68,0xFF,
    0xa0,0xa4,0x58,0xFF,0x80,0x98,0x40,0xFF,0xb4,0x8c,0x34,0xFF,0xc0,0x94,0x38,0xFF,
    0xfc,0xa4,0x00,0xFF,0xfc,0xb8,0x18,0xFF,0xfc,0xc4,0x2c,0xFF,0xfc,0xd0,0x44,0xFF,
    0xfc,0xdc,0x5c,0xFF,0xfc,0xe4,0x74,0xFF,0xfc,0xec,0x8c,0xFF,0xfc,0xf4,0xa4,0xFF,
    0x14,0x0c,0x00,0xFF,0x1c,0x10,0x00,0xFF,0x28,0x18,0x00,0xFF,0x30,0x20,0x00,0xFF,
    0x3c,0x28,0x04,0xFF,0x44,0x30,0x08,0xFF,0x50,0x3c,0x0c,0xFF,0x58,0x44,0x10,0xFF,
    0x64,0x4c,0x14,0xFF,0x70,0x54,0x18,0xFF,0x7c,0x5c,0x20,0xFF,0x88,0x68,0x28,0xFF,
    0x90,0x6c,0x28,0xFF,0x98,0x74,0x2c,0xFF,0xa4,0x7c,0x30,0xFF,0xac,0x84,0x30,0xFF,
    0x20,0x08,0x04,0xFF,0x24,0x08,0x04,0xFF,0x2c,0x0c,0x04,0xFF,0x34,0x10,0x04,0xFF,
    0x38,0x10,0x04,0xFF,0x40,0x14,0x00,0xFF,0x48,0x18,0x00,0xFF,0x50,0x1c,0x00,0xFF,
    0x5c,0x24,0x00,0xFF,0x6c,0x30,0x04,0xFF,0x78,0x3c,0x0c,0xFF,0x88,0x4c,0x14,0xFF,
    0x98,0x58,0x20,0xFF,0xa4,0x68,0x2c,0xFF,0xb4,0x78,0x38,0xFF,0xc4,0x8c,0x48,0xFF,
    0x28,0x00,0x00,0xFF,0x44,0x00,0x00,0xFF,0x64,0x00,0x00,0xFF,0x80,0x00,0x00,0xFF,
    0xa0,0x00,0x00,0xFF,0xbc,0x00,0x00,0xFF,0xdc,0x00,0x00,0xFF,0xfc,0x00,0x00,0xFF,
    0xfc,0x14,0x00,0xFF,0xfc,0x24,0x00,0xFF,0xfc,0x34,0x00,0xFF,0xfc,0x44,0x00,0xFF,
    0xfc,0x58,0x00,0xFF,0xfc,0x68,0x00,0xFF,0xfc,0x78,0x00,0xFF,0xfc,0x8c,0x00,0xFF,
    0x04,0x14,0x00,0xFF,0x04,0x18,0x00,0xFF,0x08,0x20,0x00,0xFF,0x0c,0x28,0x00,0xFF,
    0x10,0x2c,0x00,0xFF,0x14,0x34,0x00,0xFF,0x18,0x3c,0x00,0xFF,0x20,0x44,0x00,0xFF,
    0x24,0x4c,0x00,0xFF,0x2c,0x54,0x00,0xFF,0x30,0x5c,0x00,0xFF,0x38,0x64,0x04,0xFF,
    0x3c,0x6c,0x04,0xFF,0x44,0x74,0x04,0xFF,0x4c,0x7c,0x08,0xFF,0x54,0x84,0x0c,0xFF,
    0x20,0x28,0x28,0xFF,0x28,0x34,0x34,0xFF,0x30,0x40,0x40,0xFF,0x38,0x4c,0x4c,0xFF,
    0x40,0x54,0x58,0xFF,0x48,0x60,0x64,0xFF,0x50,0x6c,0x74,0xFF,0x58,0x74,0x78,0xFF,
    0x60,0x7c,0x80,0xFF,0x6c,0x84,0x88,0xFF,0x78,0x8c,0x90,0xFF,0x88,0x98,0x98,0xFF,
    0x94,0xa4,0xa4,0xFF,0xa0,0xb4,0xb4,0xFF,0xac,0xc0,0xc0,0xFF,0xb8,0xd0,0xd0,0xFF,
    0x04,0x0c,0x1c,0xFF,0x04,0x10,0x20,0xFF,0x04,0x14,0x24,0xFF,0x04,0x18,0x2c,0xFF,
    0x04,0x1c,0x30,0xFF,0x08,0x24,0x38,0xFF,0x08,0x28,0x3c,0xFF,0x08,0x30,0x40,0xFF,
    0x08,0x38,0x48,0xFF,0x10,0x40,0x60,0xFF,0x1c,0x48,0x78,0xFF,0x28,0x50,0x90,0xFF,
    0x38,0x54,0xac,0xFF,0x48,0x58,0xc4,0xFF,0x5c,0x5c,0xdc,0xFF,0x84,0x74,0xf8,0xFF,
    0x00,0x00,0x00,0xFF,0x08,0x08,0x08,0xFF,0x10,0x10,0x10,0xFF,0x1c,0x1c,0x1c,0xFF,
    0x24,0x24,0x28,0xFF,0x30,0x30,0x34,0xFF,0x3c,0x3c,0x3c,0xFF,0x44,0x44,0x48,0xFF,
    0x4c,0x4c,0x50,0xFF,0x58,0x58,0x5c,0xFF,0x64,0x64,0x68,0xFF,0x70,0x70,0x74,0xFF,
    0x7c,0x7c,0x80,0xFF,0x88,0x88,0x8c,0xFF,0x94,0x94,0x98,0xFF,0x00,0x00,0x00,0xFF
]

class Array():
    def __init__(self, cls_item):
        self.length_ = cls_item.len()
        self.cls_item = cls_item
        self.items = []

    def read(self, file):
        for i in range(self.length_):
            item = self.cls_item()
            item.read_item(file)
            self.items.append(item)
        return self

    def export(self, out):
        if len(self.items) == 0:
            return

        out.write('\n')
        doc = self.cls_item.__doc__
        out.write(self.cls_item.__doc__ + '\n')
        out.write('编号,'+self.items[0].tile_item() + '\n')
        for i in range(self.length_):
            content = self.items[i].export_item()
            out.write(f'{i},' + content + '\n')
        return self

    def __getitem__(self, i):
        return self.items[i]
        
    def __len__(self):
        return len(self.items)

class Item():
    offset : int = 0
    length : int = 0

    def use_name(self):
        return hasattr(self, 'name_')

    def read_item(self, file):
        self.offset = file.tell()
        self.read(file)
        offset = file.tell()
        self.length = offset - self.offset
    
    def tile_item(self):
        result = ''
        suf = self.tile()
        if len(suf) > 0:
            suf = ','+suf
        result = '偏移,长度' + suf
        if self.use_name:
            result = '名称,'+result
        return result
    
    def tile(self):
        return ''

    def export_item(self):
        result = ''
        suf = self.export()
        if len(suf) > 0:
            suf = ','+suf
        result = f'x{self.offset:05x},x{self.length:03x}' + suf
        if self.use_name:
            result = f'{self.name()},'+result
        return result

    def name(self):
        if hasattr(self, 'name_'):
            return self.name_
        else:
            return ''

if __name__ == "__main__":
    # export pal for shader
    for i in range(64):
        for j in range(4):
            z = i*4+j
            r = palette[4*z]/255.0
            g = palette[4*z + 1]/255.0
            b = palette[4*z + 2]/255.0
            a = palette[4*z + 3]/255.0
            print(f"vec4({r:0.6f},{g:0.6f},{b:0.6f},{a:0.6f}),", end="")
        print('')