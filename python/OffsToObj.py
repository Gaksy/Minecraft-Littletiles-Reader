import os

def read_off_file(file_path):
    """读取OFF文件，并返回顶点和面数据"""
    with open(file_path, 'r') as f:
        lines = f.readlines()

    # 确保文件以OFF格式开始
    if lines[0].strip() != 'OFF':
        raise ValueError(f"文件 {file_path} 不是有效的 OFF 文件")

    # 读取顶点数、面数和边数
    vertex_count, face_count, _ = map(int, lines[1].split())

    vertices = []
    faces = []

    # 读取顶点数据
    for i in range(2, 2 + vertex_count):
        vertex = list(map(float, lines[i].split()))
        vertices.append(vertex)

    # 读取面数据
    for i in range(2 + vertex_count, 2 + vertex_count + face_count):
        face = list(map(int, lines[i].split()))[1:]  # 第一项是面顶点数，去掉它
        faces.append(face)

    return vertices, faces

def write_obj_file(file_path, vertices, faces):
    """将数据写入OBJ格式的文件"""
    with open(file_path, 'w') as f:
        # 写入顶点数据
        for vertex in vertices:
            f.write(f"v {' '.join(map(str, vertex))}\n")

        # 写入面数据
        for face in faces:
            f.write(f"f {' '.join(str(i + 1) for i in face)}\n")  # OBJ中的索引从1开始

def convert_off_to_obj(input_directory, output_directory):
    """将OFF文件转换为OBJ文件，并保存在output_directory"""
    # 创建输出目录
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    # 遍历输入目录中的所有OFF文件
    for filename in os.listdir(input_directory):
        if filename.endswith(".off"):
            input_file = os.path.join(input_directory, filename)
            try:
                # 读取OFF文件内容
                vertices, faces = read_off_file(input_file)

                # 生成输出文件路径
                output_file = os.path.join(output_directory, filename.replace('.off', '.obj'))

                # 写入OBJ文件
                write_obj_file(output_file, vertices, faces)
                print(f"已将 {filename} 转换为 {output_file}")
            except ValueError as e:
                print(f"跳过无效的文件 {input_file}: {e}")

if __name__ == "__main__":
    input_directory = "offs"
    output_directory = "objs"

    convert_off_to_obj(input_directory, output_directory)
