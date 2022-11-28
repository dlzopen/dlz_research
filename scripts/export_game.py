import os
import struct

from numpy import number

'''Run Script in this folder'''

town = [
    "桃花岭", "赵村", "花坪镇", "李村", "土桥",
    "白洋淀", "萧家庄", "冉庄", "枣庄", "阳村",
    "清风寨", "鸡公山", "太平镇", "黑风寨", "长湾",
    "施南镇", "红岩镇", "平阳县城", "天池山"
]

JINENG_TYPE_STR = [
    "增加体力",
    "增加士气",
    "增加体力士气",
    "降低体力",
    "降低士气",
    "防御-70%",
    "攻击-70%",
    "抢夺装备",
    "攻击等级-1",
    "停止攻击一回合",
    "恢复攻击"
]

DLZ_PATH = "../DLZFULL/"

def make_dir():
    try :
        os.mkdir("../export/")
    except:
        pass
    finally: 
        pass

def get_path_in_dlz(filename):
    return DLZ_PATH + filename

class Array():
    def __init__(self, cls_item):
        self.length_ = cls_item.len()
        self.cls_item = cls_item
        self.items = []

    def read(self, file):
        for i in range(self.length_):
            item = self.cls_item()
            item.index = i
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
    offset : number = 0
    length : number = 0

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

class What0(Item):
    '''10X16矩阵，意义待确定'''
    def read(self, file):
        self.pad = list(file.read(0x10))

    def tile(self):
        return 'B0,B1,B2,B3,B4,B5,B6,B7,B8,B9,Ba,Bb,Bc,Bd,Be,Bf'

    def export(self):
        res = ''
        for i in self.pad:
            res = res + f'{i:02x},'
        return res[:-1]

    @staticmethod
    def len():
        return 10
class What1(Item):
    '''16X20矩阵，意义待确定'''
    def read(self, file):
        self.pad = list(file.read(0x14))

    def tile(self):
        return 'B0,B1,B2,B3,B4,B5,B6,B7,B8,B9,Ba,Bb,Bc,Bd,Be,Bf,Bg,Bh,Bi,Bj'

    def export(self):
        return ','.join([f'x{i:02x}' for i in self.pad])

    @staticmethod
    def len():
        return 16

class BinZhong(Item):
    '''兵种，96种'''
    def read(self, file):
        self.name_     = file.read(0x10).decode('gbk', errors='ignore').strip(' \x00')
        self.img_index = struct.unpack('I', file.read(4))[0]   # 图形索引
        self.tili      = struct.unpack('I', file.read(4))[0]   # 体力基础
        self.tili_plus = struct.unpack('I', file.read(4))[0]   # 体力加成
        self.gongji_plus = struct.unpack('I', file.read(4))[0] # 攻击加成
        self.fangyu_plus = struct.unpack('I', file.read(4))[0] # 防御加成
        self.yidong    = struct.unpack('I', file.read(4))[0]   # 移动
        self.gongji    = struct.unpack('I', file.read(4))[0]   # 攻击
        self.fangyu    = struct.unpack('I', file.read(4))[0]   # 防御
        self.select_fw = struct.unpack('I', file.read(4))[0] # 攻击范围
        self.apply_fw = struct.unpack('I', file.read(4))[0]   # 攻击形状
        self.zhuanzhi = struct.unpack('I', file.read(4))[0]   # 升职兵种，兵种索引值 0为孙悟空，既是不可以
        self.zhuanzhidaoju = struct.unpack('I', file.read(4))[0] # 转职所需道具 0为不可以
        self.pad = list(struct.unpack('20I', file.read(80)));
        self.jineng  = list(file.read(0x10)) # 技能标志位

    def tile(self):
        return '图形索引,体力基础,体力加成,攻击加成,防御加成,移动,攻击,防御,攻击选择范围,攻击应用形状,升职,转职道具,补给,激励,医疗,火攻,滚木,擂石,威慑,骚扰,破坏,掠夺,反间,疑兵,督战,暗杀,施毒,空袭'

    def export(self):
        res = (f'x{self.img_index:02x},{self.tili},{self.tili_plus},{self.gongji_plus},{self.fangyu_plus},'
                f'{self.yidong},{self.gongji},{self.fangyu},{self.select_fw},{self.apply_fw},'
                f'{self.zhuanzhi},{self.zhuanzhidaoju}')
        jn = ','.join(['*' if i else ' ' for i in self.jineng])

        return res +',' + jn

    @staticmethod
    def len():
        return 96

class BinZhongYiDong(Item):
    '''兵种移动,值为兵种（行）在地形（列）上所需要消耗的行动力'''
    def read(self, file):
        self.pad  = list(file.read(0x10)) # 地形消耗行动力16个

    def tile(self):
        return 'D0,D1,D2,D3,D4,D5,D6,D7,D8,D9,Da,Db,Dc,Dd,De,Df'

    def export(self):
        res = ''
        for i in self.pad:
            res = res +f'{i},'

        return res[:-1]

    def name(self):
        return game.get_binzhong()[self.index].name()

    @staticmethod
    def len():
        return 96

class WuPin(Item):
    '''物品，160种'''
    def read(self, file):
        self.name_     = file.read(0x10).decode('gbk', errors='ignore').strip(' \x00')
        self.uint_     = file.read(0x4).decode('gbk', errors='ignore').strip(' \x00')
        pad = file.read(0x4)
        self.img_index = struct.unpack('I', file.read(4))[0]
        self.price = struct.unpack('I', file.read(4))[0]
        self.para1 = struct.unpack('I', file.read(4))[0]
        self.para2 = struct.unpack('I', file.read(4))[0]
        self.para3 = struct.unpack('I', file.read(4))[0]
        self.para4 = struct.unpack('I', file.read(4))[0]
        self.apply = struct.unpack('I', file.read(4))[0]
        pad = file.read(0x4)
        pad = file.read(0x4)
        pad = file.read(0x4)
        pad = file.read(0x4)
        pad = file.read(0x4)

    def tile(self):
        return '单位,图片,价值,参数1,参数2,参数3,参数4,应用范围'

    def export(self):
        return f'{self.uint_},x{self.img_index:02d},{self.price},{self.para1},{self.para2},{self.para3},{self.para4},{self.apply}'

    @staticmethod
    def len():
        return 160

class DiLei(Item):
    '''地雷，10种'''
    def read(self, file):
        self.name_ = file.read(0x10).decode('gbk', errors='ignore').strip(' \x00')
        self.price = struct.unpack('I',file.read(0x4))[0]
        self.pad = file.read(0x1C)

    def tile(self):
        return '价格'

    def export(self):
        return f'{self.price}'
    
    @staticmethod
    def len():
        return 10

'''地雷配方，即每种地雷DiLei的对应制作原料Material'''
class DiLeiRecipe(Item):
    '''地雷配方，10个'''
    def read(self, file):
        self.recipe = list(file.read(10))

    def tile(self):
        return ','.join([m.name() for m in game.get_material()])

    def export(self):
        res = ''
        for i in self.recipe:
            if i != 0:
                res = res + f'{i},'
            else:
                res = res + ' ,'
        return res[:-1]

    def name(self):
        return game.get_dilei()[self.index].name()

    @staticmethod
    def len():
        return 10


'''部队可以使用的技能，共16个技能'''
class JiNeng(Item):
    '''技能，16个'''
    def read(self, file):
        self.name_ = file.read(0x10).decode('gbk', errors='ignore').strip(' \x00')
        self.para1 = struct.unpack('I',file.read(0x4))[0]
        self.para2 = struct.unpack('I',file.read(0x4))[0]
        self.type  = struct.unpack('I',file.read(0x4))[0]
        self.select= struct.unpack('I',file.read(0x4))[0]
        self.apply  = struct.unpack('I',file.read(0x4))[0]
        self.pad6  = struct.unpack('I',file.read(0x4))[0]
        self.flm   = struct.unpack('I',file.read(0x4))[0]
        self.audio = struct.unpack('I',file.read(0x4))[0]

    def tile(self):
        return '参数1,参数2,类型,选择范围，应用范围,pad6,动画,声音'

    def export(self):
        return f'{self.para1},{self.para2},x{self.type:x} {JINENG_TYPE_STR[self.type]},{self.select},{self.apply},{self.pad6},{self.flm},{self.audio}'
    
    @staticmethod
    def len():
        return 0x10

'''地雷制作原料，共10种原料'''
class Material(Item):
    '''地雷原料，10个'''
    def read(self, file):
        self.name_ = file.read(0x10).decode('gbk', errors='ignore').strip(' \x00')
        self.uint  = file.read(0x4).decode('gbk', errors='ignore').strip(' \x00')
        self.pad   = file.read(4)
        self.price = struct.unpack('I',file.read(0x4))[0]

    def tile(self):
        return '单位,单价'

    def export(self):
        return f'{self.uint},{self.price}'
    
    @staticmethod
    def len():
        return 10

class SelectFanWei(Item):
    '''选择范围，16个类型，每个类型为9X9矩阵，人在中间。兵种和技能都有选择范围索引'''
    def read(self, file):
        self.pad = file.read(9*9)

    def tile(self):
        return '0,1,2,3,4,5,6,7,8'

    def export(self):
        res = []
        for j in range(9):
            ires = '\n'.join(['*' if self.pad[i*9 + j] else '.' for i in range(9)])
            res.append('"' + ires[:-1] + '"')
        return ','.join(res)
    
    @staticmethod
    def len():
        return 16

class ApplyFanWei(Item):
    '''应用范围，48个类型，每个类型为7X7矩阵，应用点在中间。值为百分比'''
    def read(self, file):
        self.pad = struct.unpack('49I',file.read(49*4))

    def tile(self):
        return '0,1,2,3,4,5,6'

    def export(self):
        res = []
        for j in range(7):
            ires = '\n'.join([f'{self.pad[i*7 + j]:03d}' for i in range(7)])
            res.append('"' + ires + '"')
        return ','.join(res)

    @staticmethod
    def len():
        return 48

class WuPinValid(Item):
    '''物品是否可使用，列为兵种，共96个兵种'''
    def read(self, file):
        self.pad = list(file.read(0x60))

    def tile(self):
        # return ','.join([(lambda m : f'x{m:02x}')(i) for i in range(0x60)])  # 实际数据
        return ','.join([bz.name() for bz in game.get_binzhong()])   # 兵种名称

    def export(self):
        return ','.join(['1' if i else ' ' for i in self.pad])

    def name(self):
        return game.get_wupin()[self.index].name()

    @staticmethod
    def len():
        return 160


class Game():
    def __init__(self):
        self.arr = []
        with open(get_path_in_dlz('game.dat'), 'rb') as f:
            self.arr.append(Array(What0).read(f))
            self.arr.append(Array(What1).read(f))
            self.arr.append(Array(BinZhong).read(f))
            self.arr.append(Array(BinZhongYiDong).read(f))
            self.arr.append(Array(WuPin).read(f))
            self.arr.append(Array(Material).read(f))
            self.arr.append(Array(DiLeiRecipe).read(f))
            self.arr.append(Array(DiLei).read(f))
            self.arr.append(Array(JiNeng).read(f))
            self.arr.append(Array(SelectFanWei).read(f))
            self.arr.append(Array(ApplyFanWei).read(f))
            self.arr.append(Array(WuPinValid).read(f))

    def get_binzhong(self):
        return self.arr[2]

    def get_wupin(self):
        return self.arr[4]

    def get_material(self):
        return self.arr[5]

    def get_dilei(self):
        return self.arr[7]

    def get_jineng(self):
        return self.arr[8]

    def export(self, out):
        for i in self.arr:
            i.export(out)

game = Game()

if __name__ == "__main__":
    make_dir()
    out = open('../export/game.csv', 'w+')
    out.write('运行脚本export_game.py的运行结果，该表格基于game.dat的数据分析\n')
    game.export(out)
