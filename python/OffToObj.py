def off_to_obj(off_filename, obj_filename):
    with open(off_filename, 'r') as off_file:
        lines = off_file.readlines()

    # Skipping the header "OFF"
    if lines[0].strip() != "OFF":
        print("Not a valid OFF file.")
        return

    # Read the vertex and face count from the header
    num_vertices, num_faces, _ = map(int, lines[1].split())

    vertices = []
    faces = []

    # Read vertices
    for i in range(2, 2 + num_vertices):
        vertices.append(list(map(float, lines[i].split())))

    # Read faces
    for i in range(2 + num_vertices, 2 + num_vertices + num_faces):
        face_data = lines[i].split()[1:]  # Skip the face size (usually 3 for triangles)
        # Convert vertex names like 'v3' to integer index (e.g., 'v3' -> 2)
        face_indices = [int(v[1:]) - 1 for v in face_data]
        faces.append(face_indices)

    # Write to OBJ file
    with open(obj_filename, 'w') as obj_file:
        # Write vertices
        for v in vertices:
            obj_file.write(f"v {v[0]} {v[1]} {v[2]}\n")

        # Write faces
        for f in faces:
            obj_file.write(f"f {' '.join(str(i + 1) for i in f)}\n")

# Example usage
off_filename = "test.off"
obj_filename = "output.obj"
off_to_obj(off_filename, obj_filename)
