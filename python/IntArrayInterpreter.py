from typing import List

ANGLE_EUN: int = 0
ANGLE_EUS: int = 1
ANGLE_EDN: int = 2
ANGLE_EDS: int = 3
ANGLE_WUN: int = 4
ANGLE_WUS: int = 5
ANGLE_WDN: int = 6
ANGLE_WDS: int = 7


class Coord3D:
    def __init__(self, x:int = 0, y:int = 0, z:int = 0):
        self.x:int = x
        self.y:int = y
        self.z:int = z

    def __str__(self):
        return f'({self.x}, {self.y}, {self.z})'


class AngleOffset:
    def __init__(self):
        self.x_enable:bool = False
        self.y_enable:bool = False
        self.z_enable:bool = False
        self.x_offset:int = 0
        self.y_offset:int = 0
        self.z_offset:int = 0

    def __str__(self):
        x_str:str = f"x offset: {self.x_offset}" if self.x_enable else "x offset disabled"
        y_str:str = f"y offset: {self.y_offset}" if self.y_enable else "y offset disabled"
        z_str:str = f"z offset: {self.z_offset}" if self.z_enable else "z offset disabled"
        return f'{x_str}\n{y_str}\n{z_str}'

class FlippedData:
    def __init__(self):
        self.down:bool = False
        self.up:bool = False
        self.north:bool = False
        self.south:bool = False
        self.east:bool = False
        self.west:bool = False

    def __str__(self):
        down_str = '1' if self.down else '0'
        up_str = '1' if self.up else '0'
        north_str = '1' if self.north else '0'
        south_str = '1' if self.south else '0'
        east_str = '1' if self.east else '0'
        west_str = '1' if self.west else '0'

        return f"|E|W|S|N|U|D|\n|{east_str}|{west_str}|{south_str}|{north_str}|{up_str}|{down_str}|"

class BoxData:
    def __init__(self,
                 pos_1:Coord3D = Coord3D(),
                 pos_2:Coord3D = Coord3D()
                 ):
        self.pos_1:Coord3D = pos_1
        self.pos_2:Coord3D = pos_2
        self.flipped_dara: FlippedData = FlippedData()
        self.angle_offset_data:List[AngleOffset] = []
        for i in range(0, 9):
            self.angle_offset_data.append(AngleOffset())

    def __str__(self):
        angle_offset_str = f"EUN\n{self.angle_offset_data[ANGLE_EUN].__str__()}\nEUS\n{self.angle_offset_data[ANGLE_EUS].__str__()}\nEDN\n{self.angle_offset_data[ANGLE_EDN].__str__()}\nEDS\n{self.angle_offset_data[ANGLE_EDS].__str__()}\nWUN\n{self.angle_offset_data[ANGLE_WUN].__str__()}\nWUS\n{self.angle_offset_data[ANGLE_WUS].__str__()}\nWDN\n{self.angle_offset_data[ANGLE_WDN].__str__()}\nWDS\n{self.angle_offset_data[ANGLE_WDS].__str__()}"
        return f"pos_1: {self.pos_1.__str__()}\npos_2: {self.pos_2.__str__()}\nFLIPPED DATA:\n{self.flipped_dara.__str__()}\n{angle_offset_str}"

def formatBoxStr(box_str: str):
    format_1 = box_str[1:-1].split(';')[1]
    format_2 = format_1.split(',')

    pos_1:Coord3D = Coord3D(0,0,0)
    pos_2:Coord3D = Coord3D(0,0,0)

    pos_1.x = int(format_2[0])
    pos_1.y = int(format_2[1])
    pos_1.z = int(format_2[2])
    pos_2.x = int(format_2[3])
    pos_2.y = int(format_2[4])
    pos_2.z = int(format_2[5])

    box_data: BoxData = BoxData(pos_1, pos_2)
    offset_info = int(format_2[6]) & 0xFFFFFFFF

    # offset data
    if (offset_info & 0x20000000) != 0:
        box_data.flipped_dara.east = True
    if (offset_info & 0x10000000) != 0:
        box_data.flipped_dara.west = True
    if (offset_info & 0x8000000) != 0:
        box_data.flipped_dara.south = True
    if (offset_info & 0x4000000) != 0:
        box_data.flipped_dara.north = True
    if (offset_info & 0x2000000) != 0:
        box_data.flipped_dara.up = True
    if (offset_info & 0x1000000) != 0:
        box_data.flipped_dara.down = True

    # angle data
    offset_data:List[int] = []
    for ore_offset in format_2[7:]:
        bit32 = int(ore_offset) & 0xFFFFFFFF
        # 提取高16位和低16位
        high16 = (bit32 >> 16) & 0xFFFF
        low16 = bit32 & 0xFFFF

        # 对高16位进行符号扩展
        if high16 & 0x8000:  # 如果最高位是1（负数）
            high16 = high16 - 0x10000  # 符号扩展为有符号整数

        # 对低16位进行符号扩展
        if low16 & 0x8000:  # 如果最高位是1（负数）
            low16 = low16 - 0x10000  # 符号扩展为有符号整数

        offset_data.append(high16)
        offset_data.append(low16)

    offset_data_index:int = len(offset_data) - 1
    print(offset_data)
    # WDS
    if (offset_info & 0x800000) != 0:
        box_data.angle_offset_data[ANGLE_WDS].z_enable = True
        box_data.angle_offset_data[ANGLE_WDS].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x400000) != 0:
        box_data.angle_offset_data[ANGLE_WDS].y_enable = True
        box_data.angle_offset_data[ANGLE_WDS].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x200000) != 0:
        box_data.angle_offset_data[ANGLE_WDS].x_enable = True
        box_data.angle_offset_data[ANGLE_WDS].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x100000) != 0:
        box_data.angle_offset_data[ANGLE_WDN].z_enable = True
        box_data.angle_offset_data[ANGLE_WDN].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x80000) != 0:
        box_data.angle_offset_data[ANGLE_WDN].y_enable = True
        box_data.angle_offset_data[ANGLE_WDN].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x40000) != 0:
        box_data.angle_offset_data[ANGLE_WDN].x_enable = True
        box_data.angle_offset_data[ANGLE_WDN].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x20000) != 0:
        box_data.angle_offset_data[ANGLE_WUS].z_enable = True
        box_data.angle_offset_data[ANGLE_WUS].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x10000) != 0:
        box_data.angle_offset_data[ANGLE_WUS].y_enable = True
        box_data.angle_offset_data[ANGLE_WUS].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x8000) != 0:
        box_data.angle_offset_data[ANGLE_WUS].x_enable = True
        box_data.angle_offset_data[ANGLE_WUS].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x4000) != 0:
        box_data.angle_offset_data[ANGLE_WUN].z_enable = True
        box_data.angle_offset_data[ANGLE_WUN].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x2000) != 0:
        box_data.angle_offset_data[ANGLE_WUN].y_enable = True
        box_data.angle_offset_data[ANGLE_WUN].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x1000) != 0:
        box_data.angle_offset_data[ANGLE_WUN].x_enable = True
        box_data.angle_offset_data[ANGLE_WUN].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x800) != 0:
        box_data.angle_offset_data[ANGLE_EDS].z_enable = True
        box_data.angle_offset_data[ANGLE_EDS].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x400) != 0:
        box_data.angle_offset_data[ANGLE_EDS].y_enable = True
        box_data.angle_offset_data[ANGLE_EDS].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x200) != 0:
        box_data.angle_offset_data[ANGLE_EDS].x_enable = True
        box_data.angle_offset_data[ANGLE_EDS].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x100) != 0:
        box_data.angle_offset_data[ANGLE_EDN].z_enable = True
        box_data.angle_offset_data[ANGLE_EDN].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x80) != 0:
        box_data.angle_offset_data[ANGLE_EDN].y_enable = True
        box_data.angle_offset_data[ANGLE_EDN].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x40) != 0:
        box_data.angle_offset_data[ANGLE_EDN].x_enable = True
        box_data.angle_offset_data[ANGLE_EDN].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x20) != 0:
        box_data.angle_offset_data[ANGLE_EUS].z_enable = True
        box_data.angle_offset_data[ANGLE_EUS].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x10) != 0:
        box_data.angle_offset_data[ANGLE_EUS].y_enable = True
        box_data.angle_offset_data[ANGLE_EUS].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x8) != 0:
        box_data.angle_offset_data[ANGLE_EUS].x_enable = True
        box_data.angle_offset_data[ANGLE_EUS].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1

    if (offset_info & 0x4) != 0:
        box_data.angle_offset_data[ANGLE_EUN].z_enable = True
        box_data.angle_offset_data[ANGLE_EUN].z_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x2) != 0:
        box_data.angle_offset_data[ANGLE_EUN].y_enable = True
        box_data.angle_offset_data[ANGLE_EUN].y_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    if (offset_info & 0x1) != 0:
        box_data.angle_offset_data[ANGLE_EUN].x_enable = True
        box_data.angle_offset_data[ANGLE_EUN].x_offset = offset_data[offset_data_index]
        offset_data_index -= 1
    print(offset_data_index)
    return box_data
def main():
    box_str:str = input("box str:")
    box_data:BoxData = formatBoxStr(box_str)
    print(box_data)
    grid = input("grid:")
    matlab_str = f"""
% 定义网格大小
grid_type = {grid};

% 定义立方体的八个顶点
block_aabb = LtBlock(0,0,0,{grid},{grid},{grid});
block = LtBlock({box_data.pos_1.x},{box_data.pos_1.y},{box_data.pos_1.z},{box_data.pos_2.x},{box_data.pos_2.y},{box_data.pos_2.z});
block.applyOffset(AngleID.WDS, LtPoint({box_data.angle_offset_data[ANGLE_WDS].x_offset}, {box_data.angle_offset_data[ANGLE_WDS].y_offset}, {box_data.angle_offset_data[ANGLE_WDS].z_offset}));
block.applyOffset(AngleID.WDN, LtPoint({box_data.angle_offset_data[ANGLE_WDN].x_offset}, {box_data.angle_offset_data[ANGLE_WDN].y_offset}, {box_data.angle_offset_data[ANGLE_WDN].z_offset}));
block.applyOffset(AngleID.WUS, LtPoint({box_data.angle_offset_data[ANGLE_WUS].x_offset}, {box_data.angle_offset_data[ANGLE_WUS].y_offset}, {box_data.angle_offset_data[ANGLE_WUS].z_offset}));
block.applyOffset(AngleID.WUN, LtPoint({box_data.angle_offset_data[ANGLE_WUN].x_offset}, {box_data.angle_offset_data[ANGLE_WUN].y_offset}, {box_data.angle_offset_data[ANGLE_WUN].z_offset}));
block.applyOffset(AngleID.EDS, LtPoint({box_data.angle_offset_data[ANGLE_EDS].x_offset}, {box_data.angle_offset_data[ANGLE_EDS].y_offset}, {box_data.angle_offset_data[ANGLE_EDS].z_offset}));
block.applyOffset(AngleID.EDN, LtPoint({box_data.angle_offset_data[ANGLE_EDN].x_offset}, {box_data.angle_offset_data[ANGLE_EDN].y_offset}, {box_data.angle_offset_data[ANGLE_EDN].z_offset}));
block.applyOffset(AngleID.EUS, LtPoint({box_data.angle_offset_data[ANGLE_EUS].x_offset}, {box_data.angle_offset_data[ANGLE_EUS].y_offset}, {box_data.angle_offset_data[ANGLE_EUS].z_offset}));
block.applyOffset(AngleID.EUN, LtPoint({box_data.angle_offset_data[ANGLE_EUN].x_offset}, {box_data.angle_offset_data[ANGLE_EUN].y_offset}, {box_data.angle_offset_data[ANGLE_EUN].z_offset}));
% 绘制
showGrid(grid_type, -2, 2);
patchLtBlock(block, 'green', 0.7);
patchLtBlock(block_aabb, 'blue', 0.1);

%cyan
    """
    print(matlab_str)
main()
