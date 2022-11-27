'''地雷战算法复原算法'''

def bit_count(v:int) -> int:
    return sum([ (v & 1 << i) != 0 for i in range(16)])

class Routing():
    '''该算法的地址是4198E5，复原算法对其进行了简化'''

    @staticmethod
    def calc_dlz(from_x, from_y, to_x, to_y):
        return Routing.calc(to_x - from_x, to_y - from_y)

    @staticmethod
    def calc(dx, dy):
        l = abs(dx) + abs(dy)
        path_list = []
        t:int = pow(2, l)

        for p in range(t):
            if bit_count(p) != abs(dy):
                path_list.append((False))
            else:
                d = ''
                cost = 0
                for b in range(l):
                    if (p & (1 << b)) == 0:
                        d += 'L' if dx > 0 else 'R'
                    else:
                        d += 'D' if dy > 0 else 'U'

                path_list.append((True, d, Routing.cost(d, l)))

        return path_list

    @staticmethod
    def cost(path_description, path_length):
        return path_length

if __name__ == "__main__":
    pl = Routing.calc_dlz(10, 21, 8, 19)
    [print(p) for p in pl]
