
import os
import struct

from export_utils import *

class Fight(Item):
    '''战斗，47场'''
    def read(self, file):
        self.name_   = file.read(0x20).decode('gbk', errors='ignore').strip(' \x00')
        self.fix_name()
        pad          = file.read(0x4)
        pad          = file.read(0x4)
        self.success = file.read(0x40).decode('gbk', errors='ignore').strip(' \x00')
        self.failure = file.read(0x40).decode('gbk', errors='ignore').strip(' \x00')
        pad          = file.read(0x4F74)

    # 处理名称里面的BUG
    def fix_name(self):
        i = 0
        while i < len(self.name_):
            if self.name_[i] == '\0':
                break
            else:
                i = i+1
        self.name_ = self.name_[:i]

    def tile(self):
        return ''

    def export(self):
        return ''

    @staticmethod
    def len():
        return 47

class Fights():
    def __init__(self) -> None:
        self.arr = []
        with open(get_path_in_dlz('fight.dat'), 'rb') as f:
            self.arr.append(Array(Fight).read(f))
            
    def get_fight(self):
        return self.arr[0]

    def export(self, out):
        for i in self.arr:
            i.export(out)

fights = Fights()

if __name__ == "__main__":
    make_dir(DLZ_EXPORT)
    make_dir()
    out = open('../export/fight.csv', 'w+')
    out.write('运行脚本export_fight.py的运行结果，该表格基于fight.dat的数据分析\n')
    out.write('你可以在这里发现它：https://gitee.com/ltg1831/dlz/commits/master/doc/scripts/export_fight.py\n')
    fights.export(out)