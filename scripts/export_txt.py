
'''Run Script in this folder'''

import os
from export_utils import *

def get_path_in_dlz(filename):
    return DLZ_PATH + filename

class Txt():
    def __init__(self):
        self.buf=[]
        self.data = []
        self.txt = ['']*640
        out = open('../export/txt.dat', 'wb')
        with open(get_path_in_dlz('txt.dat'), 'rb') as f:
            self.buf = list(f.read())
            self.data = [0]*len(self.buf)
            i = 0
            v6 = 0
            v7 = 0
            v5 = 0
            v9 = 0
            v9 = 0
            while i < len(self.buf):
                v2 = self.buf[i]
                v8 = v2 ^ 0x51

                out.write(chr(v8).encode('latin1'))
                i = i+1

if __name__ == "__main__":
    make_dir(DLZ_EXPORT)
    txt = Txt()