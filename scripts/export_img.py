from PIL import Image
import os
import struct
from export_utils import *

class Img():
    def __init__(self, filename):
        self.buf = []
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
    
    def export(self):
        path = DLZ_EXPORT + self.filename + '.png'

        img = Image.new("RGBA", (self.width, self.height))
        for i in range(0, 1):
            img.putdata(self.get_data(i))
            # save new image
            img.save(f"{path}")
            # img.show()

    # def export(self):
    #     path = DLZ_EXPORT + self.filename + '.png'

    #     img = Image.new("P", (self.width, self.height))
    #     img.palette = get_image_palette(False)
    #     for i in range(0, 1):
    #         img.putdata(self.buf)
    #         # save new image
    #         img.save(f"{path}")
    #         # img.show()

def export(filename):
    img = Img(filename)
    make_dir(DLZ_EXPORT)
    img.export()

if __name__ == "__main__":
    export('77-1.IMG')
    export('77-2.IMG')
    export('918-2.IMG')
    export('918-3.IMG')
    export('BACK19.IMG')
    export('BG01.IMG')
    export('BG02.IMG')
    export('BG03.IMG')
    export('BIGMAP.IMG')
    export('BIGMINE0.IMG')
    export('BIGMINE1.IMG')
    export('BOX.IMG')
    export('BUTTON0.IMG')
    export('BUTTON00.IMG')
    export('BUTTON01.IMG')
    export('BUTTON1.IMG')
    export('BUTTON10.IMG')
    export('BUTTON11.IMG')
    export('BUTTON12.IMG')
    export('BUTTON13.IMG')
    export('BUTTON2.IMG')
    export('BUTTON22.IMG')
    export('BUTTON3.IMG')
    export('BUTTON4.IMG')
    export('BUTTON5.IMG')
    export('BUTTON6.IMG')
    export('BUTTON7.IMG')
    export('BUTTON8.IMG')
    export('BUTTON9.IMG')
    export('DLZ00.IMG')
    export('END00.IMG')
    export('END01.IMG')
    export('END02.IMG')
    export('END03.IMG')
    # export('END04.IMG') // 320 X 200
    export('END05.IMG')
    export('FRAME00.IMG')
    export('JZ-1.IMG')
    export('JZ-2.IMG')
    export('K00.IMG')
    export('K01.IMG')
    export('K02.IMG')
    export('K03.IMG')
    export('K04.IMG')
    export('KUAN04.IMG')
    export('KUANG00.IMG')
    export('KUANG01.IMG')
    export('KUANG04.IMG')
    export('MENU00.IMG')
    export('MENU01.IMG')
    export('MENU02.IMG')
    export('MENU03.IMG')
    export('MENU08.IMG')
    export('MENU09.IMG')
    export('MENU10.IMG')
    export('MENU20.IMG')
    export('MENU21.IMG')
    export('MENU22.IMG')
    export('MENU23.IMG')
    export('MENU24.IMG')
    export('MENU30.IMG')
    export('MENU34.IMG')
    export('MENU50.IMG')
    export('MENU51.IMG')
    export('MENU52.IMG')
    export('MENU53.IMG')
    export('MENU54.IMG')
    export('MINE.IMG')
    export('MINE01.IMG')
    export('MINE02.IMG')
    export('NPIC01.IMG')
    export('NPIC02.IMG')
    export('NPIC03.IMG')
    export('NPIC04.IMG')
    export('NPIC05.IMG')
    export('NPIC06.IMG')
    export('NPIC07.IMG')
    export('NPIC08.IMG')
    export('NPIC18.IMG')
    export('NPIC19.IMG')
    export('NPIC20.IMG')
    export('NPIC21.IMG')
    export('NPIC22.IMG')
    export('NPIC25.IMG')
    export('NPIC29.IMG')
    export('ZYB.IMG')
