from PIL import Image, ImageFont, ImageDraw
import os
import struct
from export_utils import *
from export_fight import fights

is_transparent = True

'''Run Script in this folder'''

class BLK():
    def __init__(self):
        self.index = []
        self.blk = []
        self.pal = get_palette(is_transparent)
        self.drawline = False

        with open(get_path_in_dlz('blk.dat'), 'rb') as f:
            for i in range(0x40):
                offset = struct.unpack('I', f.read(4))[0]
                length = struct.unpack('I', f.read(4))[0]
                self.index.append([offset, length])

            blk_index = 0
            while blk_index < 0x40:
                size = 0
                blk_data = []
                while size < self.index[blk_index][1] :
                    # 48*48
                    blk_data.append(list(f.read(48*48)))
                    size = size + 48 * 48
                blk_index = blk_index + 1
                self.blk.append(blk_data)
        
    def get_tile(self, blk_index, blk_data_index, drawline):
        p = self.pal
        res = [(p[4*d+0], p[4*d+1], p[4*d+2], p[4*d+3]) for d in self.blk[blk_index][blk_data_index]]
        if drawline:
            for i in range(48):
                p = 47 + i * 48
                res [p] = (res[p][0], res[p][1], res[p][2], 0)

            for i in range(48):
                p = 47 *48 + i
                res [p] = (res[p][0], res[p][1], res[p][2], 0)
        return res

    def export_tile(self, data, tile_data, width, height, x, y):
        for j in range(TILE_HEIGHT):
            data_x = x * TILE_WIDTH
            data_y = y * TILE_HEIGHT + j
            data_z = data_y * width * TILE_WIDTH + data_x
            for i in range(TILE_WIDTH):
                data[data_z + i ] = tile_data[j * TILE_HEIGHT + i]

    def export_blk(self):
        path = "../export/blk"
        os.makedirs(path, exist_ok=True)
        p = self.pal

        blk_index = 0

        height = 9
        for blk_index in range(0x40):
            item = self.index[blk_index]
            count = item[1] / (48 * 48)
            if count == 0:
                continue

            width = (int)(count / height + (int)((count % height) != 0))

            img = Image.new("RGBA", (48*width, 48*height))
            data = [(0, 0,0, 0)]* (48*width * 48*height)

            for y in range(height):
                for x in range(width):
                    tile_index = y * width + x
                    if tile_index >= count:
                        break

                    tile_data = self.get_tile(blk_index, tile_index, self.drawline)

                    self.export_tile(data, tile_data, width, height, (int)(tile_index / height), (int)(tile_index % height))

            img.putdata(data)
            save_img(img, f"../export/blk/{blk_index:02d}.png")

        # blk_index = 11

        # height = 9
        # for height in range(1, 0x32):
        #     item = self.index[blk_index]
        #     count = item[1] / (48 * 48)
        #     if count == 0:
        #         continue

        #     width = (int)(count / height + (int)((count % height) != 0))

        #     img = Image.new("RGBA", (48*width, 48*height))
        #     data = [(0, 0,0, 0)]* (48*width * 48*height)

        #     for y in range(height):
        #         for x in range(width):
        #             tile_index = y * width + x
        #             if tile_index >= count:
        #                 break

        #             tile_data = self.get_tile(blk_index, tile_index, self.drawline)

        #             self.export_tile(data, tile_data, width, height, (int)(tile_index / height), (int)(tile_index % height))

        #     img.putdata(data)
        #     save_img(img, f"../export/blk/{blk_index:02d}_{height:02d}.png")

class Map():
    class Item ():
        width = 0
        height = 0
        blk_file = ''
        pal_file = ''
        buffer = []

    def __init__(self):
        self.index = []
        self.map = []
        self.drawline = False
        self.drawflag = False

        with open(get_path_in_dlz('map.dat'), 'rb') as f:
            for i in range(0x40):
                offset = struct.unpack('I', f.read(4))[0]
                length = struct.unpack('I', f.read(4))[0]
                self.index.append([offset, length])

            map_index = 0
            for map_index in range(47):
                map_item = Map.Item()
                map_item.width = struct.unpack('H', f.read(2))[0]
                map_item.height = struct.unpack('H', f.read(2))[0]
                map_item.blk_file = f.read(12).decode('gbk', errors='ignore').strip(' \x00')
                map_item.pal_file = f.read(12).decode('gbk', errors='ignore').strip(' \x00')
                l = map_item.width * map_item.height
                map_item.buffer = list(struct.unpack(f'{l}H', f.read(2*l)))
                self.map.append(map_item)

    def export_tile(self, map_item, data, tile_data, x, y):
        for j in range(TILE_HEIGHT):
            data_x = x * TILE_WIDTH
            data_y = y * TILE_HEIGHT + j
            data_z = data_y * map_item.width * TILE_WIDTH + data_x
            for i in range(TILE_WIDTH):
                data[data_z + i ] = tile_data[j * TILE_HEIGHT + i]

    def draw_flag(self, item, x, y):
        tile_pos = y * item.width + x
        tile_flag = (item.buffer[tile_pos] & 0xf000) >> 12
        self.draw.text((x*48 + 12,y*48+12), f'{tile_flag:0x}',font = self.font, fill = 'white')

    def export_map(self):
        path = "../export/map"
        os.makedirs(path, exist_ok=True)
        
        self.font = ImageFont.load_default()
        
        for map_index in range(47):
            item = self.map[map_index]

            img = Image.new("RGBA", (item.width*TILE_WIDTH, item.height*TILE_HEIGHT))
            data = [(0, 0,0, 0)]* (item.width*TILE_WIDTH * item.height*TILE_HEIGHT)

            fight_name = fights.get_fight()[map_index].name()

            blk_index = (ord(item.blk_file[4]) - 0x30) * 10 + (ord(item.blk_file[5]) - 0x30)

            for y in range(item.height):
                for x in range(item.width):
                    tile_pos = y * item.width + x
                    tile_index = item.buffer[tile_pos]
                    tile_index &= 0xfff
                    tile_data = blk.get_tile(blk_index, tile_index, self.drawline)

                    self.export_tile(item, data, tile_data, x, y)

            img.putdata(data)
            if self.drawflag:
                self.draw = ImageDraw.Draw(img)
                for y in range(item.height):
                    for x in range(item.width):
                        self.draw_flag(item, x, y)
            save_img(img, f"../export/map/{map_index:02d}_{fight_name}_{blk_index:02d}.png")

blk = BLK()
map = Map()

def export_pal():
    path = DLZ_EXPORT + 'map/'
    make_dir(path)
    img = Image.new("RGBA", (16, 16))
    img.putdata([(palette[4*i+0], palette[4*i+1], palette[4*i+2], palette[4*i+3]) for i in range(0, 256)])
    img.save(f'{path}palette.png')

if __name__ == "__main__":
    make_dir(DLZ_EXPORT)
    export_pal()
    map.export_map()
    blk.export_blk()

