def off_to_obj(off_data):
    # 解析 OFF 数据
    lines = off_data.splitlines()

    # 获取顶点数量和面数量
    vertex_count, face_count, _ = map(int, lines[1].split())

    # 获取顶点数据
    vertices = [line.strip() for line in lines[2:2 + vertex_count]]

    # 获取面数据
    faces = [line.strip() for line in lines[2 + vertex_count:]]

    # 转换顶点数据为 OBJ 格式
    obj_data = []
    for vertex in vertices:
        obj_data.append(f"v {vertex}")

    # 转换面数据为 OBJ 格式
    for face in faces:
        face_indices = list(map(int, face.split()[1:]))  # 跳过面数据中的第一个数字（3）
        # 由于 OBJ 使用 1-based 索引，而 OFF 使用 0-based 索引，需要加 1
        obj_data.append(f"f {' '.join(str(idx + 1) for idx in face_indices)}")

    # 输出 OBJ 格式数据
    return "\n".join(obj_data)


# 给定的 OFF 数据（字面量字符串）
off_string = """
32 60 0
-7 4 14.1667
-6.375 4 15
-7 4 14
-7 4.16667 14.1667
-7 4.5 14.1667
-7 4.5 14
-6.75 4.5 14
-6.625 4.5 14
-6.625 4.375 14
-6.625 4 14
-6 4 15
-6.375 4.375 15
-6.375 4.5 15
-6.25 4.5 15
-6 4.5 15
-6 4.5 14.8333
-6 4.16667 14.8333
-6 4 14.8333
-7 4.5 13.5
-7.5 4.5 13.5
-7.5 4 13.5
-7 4 13.5
-5.5 4 15.5
-6 4.5 15.5
-5.5 4.5 15.5
-6 4 15.5
-7 5 15
-7 5 14
-6 5 15
-6 5 14
-6 4 14
-7 4 15
3  4 3 19
3  18 19 21
3  21 19 20
3  2 21 20
3  18 8 7
3  18 6 5
3  5 4 19
3  18 5 19
3  21 9 8
3  21 8 18
3  18 7 6
3  3 20 19
3  0 2 20
3  3 0 20
3  9 21 2
3  1 11 25
3  23 24 25
3  25 24 22
3  10 25 22
3  24 15 16
3  24 16 22
3  23 12 13
3  22 16 17
3  23 13 14
3  14 15 24
3  23 14 24
3  11 12 23
3  1 25 10
3  11 23 25
3  17 10 22
3  0 10 2
3  0 1 10
3  17 2 10
3  17 9 2
3  16 30 17
3  5 27 4
3  0 3 31
3  26 28 31
3  11 31 12
3  29 27 30
3  27 29 26
3  26 29 28
3  1 0 31
3  26 4 27
3  4 26 3
3  28 14 13
3  28 12 31
3  11 1 31
3  28 13 12
3  9 30 8
3  8 30 7
3  5 6 27
3  7 30 27
3  6 7 27
3  15 28 29
3  15 14 28
3  16 15 29
3  30 16 29
3  9 17 30
3  3 26 31
"""

# 转换并打印 OBJ 数据
obj_string = off_to_obj(off_string)
print(obj_string)
