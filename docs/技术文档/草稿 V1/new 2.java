public synchronized VectorFanCache requestCache() {
	// 如果缓存已经存在，则尝试获取缓存中的 VectorFanCache 对象
	if (cache != null) {
		VectorFanCache temp = cache.get();
		if (temp != null)
			// 如果缓存中的对象不为空，则返回该对象
			return temp;
	}
	
	// 创建一个新的 VectorFanCache 对象
	VectorFanCache cache = new VectorFanCache();
	
	// Cache axis aligned faces
	// 缓存与轴对齐的平面（面片）
	NormalPlaneF[] planes = new NormalPlaneF[Facing.values().length];
	for (int i = 0; i < planes.length; i++) {
		Facing facing = Facing.values()[i];
		Axis axis = facing.axis;
		
		// 创建一个新的 NormalPlaneF 对象，并设置它的起点和法线向量
		NormalPlaneF plane = new NormalPlaneF(new Vec3f(), new Vec3f());
		plane.origin.set(0, 0, 0);
		plane.origin.set(axis, get(facing));
		
		plane.normal.set(0, 0, 0);
		plane.normal.set(axis, facing.offset());
		
		planes[i] = plane;
		
		// 初始化缓存中对应的面片缓存
		cache.faces[i] = new VectorFanFaceCache();
	}
	
	// 创建一个数组来存储倾斜的平面（用于后续裁剪）
	NormalPlaneF[] tiltedPlanes = new NormalPlaneF[Facing.values().length * 2]; // Stores all tilted planes to use them for cutting later
	
	// 获取倾斜的角点
	// Tilted strips against axis box
	Vec3f[] corners = getTiltedCorners();
	LittleVec[] boxCorners = getCorners();
	for (int i = 0; i < Facing.values().length; i++) {
		Facing facing = Facing.values()[i];
		
		// 获取与当前朝向对应的 BoxFace
		BoxFace face = BoxFace.get(facing);
		boolean inverted = getFlipped(facing);
		
		// 获取当前朝向的面片缓存
		VectorFanFaceCache faceCache = cache.faces[i];
		
		// 获取面片的两个三角形，并计算它们的法线
		BoxCorner[] first = face.getTriangleFirst(inverted);
		Vec3f firstNormal = BoxFace.getTraingleNormal(first, corners);
		boolean firstSame = checkEqual(corners, boxCorners, first, facing.axis);
		
		BoxCorner[] second = face.getTriangleSecond(inverted);
		Vec3f secondNormal = BoxFace.getTraingleNormal(second, corners);
		boolean secondSame = checkEqual(corners, boxCorners, second, facing.axis);
		
		// 如果两个三角形都相同，则跳过当前面片
		if (firstSame && secondSame)
			continue;
		
		// 归一化法线
		//BoxFace.ensureSameLength(firstNormal, secondNormal);
		firstNormal.normalize();
		secondNormal.normalize();
		
		// 判断两个法线是否平行
		boolean parallel = firstNormal.epsilonEquals(secondNormal, VectorFan.EPSILON);
		if (parallel) {
			if (!firstSame && !firstNormal.epsilonEquals(ZERO, VectorFan.EPSILON)) {
				// 如果第一个三角形不完全相同且法线不为零向量，则创建倾斜的面片条带
				faceCache.tiltedStrip1 = createStrip(face.corners, corners);
				if (faceCache.tiltedStrip1 != null)
					tiltedPlanes[i * 2] = new NormalPlaneF(corners[first[0].ordinal()], firstNormal);
			}
		} else {
			// 如果法线不平行，则分别处理第一个和第二个三角形
			if (!firstSame && !firstNormal.epsilonEquals(ZERO, VectorFan.EPSILON)) {
				faceCache.tiltedStrip1 = createStrip(first, corners);
				if (faceCache.tiltedStrip1 != null)
					tiltedPlanes[i * 2] = new NormalPlaneF(corners[first[0].ordinal()], firstNormal);
			}
			
			if (!secondSame && !secondNormal.epsilonEquals(ZERO, VectorFan.EPSILON)) {
				faceCache.tiltedStrip2 = createStrip(second, corners);
				if (faceCache.tiltedStrip2 != null)
					tiltedPlanes[i * 2 + 1] = new NormalPlaneF(corners[second[0].ordinal()], secondNormal);
			}
		}
		
		// 移除无效的面片（那些仅为一条线的面片）
		// Remove invalid generated faces <- which are just a line
		if (faceCache.tiltedStrip1 != null && tiltedPlanes[i * 2].isInvalid()) {
			faceCache.tiltedStrip1 = null;
			tiltedPlanes[i * 2] = null;
		}
		if (faceCache.tiltedStrip2 != null && tiltedPlanes[i * 2 + 1].isInvalid()) {
			faceCache.tiltedStrip2 = null;
			tiltedPlanes[i * 2 + 1] = null;
		}
		
		// 如果两个倾斜条带都存在，则检查它们是否朝向轴向条带的内部
		if (faceCache.tiltedStrip1 != null && faceCache.tiltedStrip2 != null) {
			for (int j = 0; j < faceCache.tiltedStrip2.count(); j++) {
				Vec3f vec = faceCache.tiltedStrip2.get(j);
				if (BooleanUtils.isTrue(tiltedPlanes[i * 2].isInFront(vec, VectorFan.EPSILON))) {
					faceCache.convex = false; // If path strips face inwards the axis strip has to be copied and each cut by one plane
					break;
				}
			}
		}
		
		// 裁剪倾斜条带与轴对齐的平面
		for (int j = 0; j < planes.length; j++) {
			if (faceCache.tiltedStrip1 != null)
				faceCache.tiltedStrip1 = faceCache.tiltedStrip1.cut(planes[j]);
			if (faceCache.tiltedStrip2 != null)
				faceCache.tiltedStrip2 = faceCache.tiltedStrip2.cut(planes[j]);
		}
		
		// 对倾斜的条带进行排序
		faceCache.sortTiltedStrips(cache, tiltedPlanes[i * 2], tiltedPlanes[i * 2 + 1]);
	}
	
	// 处理与变换后的盒子相交的轴向条带
	// Axis strips against transformed box;
	for (int i = 0; i < Facing.values().length; i++) {
		Facing facing = Facing.values()[i];
		BoxFace face = BoxFace.get(facing);
		
		VectorFanFaceCache axisFaceCache = cache.faces[i];
		axisFaceCache.axisStrips.add(new VectorFan(getVecArray(face.corners)));
		
		for (int j = 0; j < Facing.values().length; j++) {
			VectorFanFaceCache faceCache = cache.faces[j];
			if (faceCache.tiltedStrip1 == null && faceCache.tiltedStrip2 == null) {
				NormalPlaneF cutPlane1 = tiltedPlanes[j * 2];
				NormalPlaneF cutPlane2 = tiltedPlanes[j * 2 + 1];
				if (faceCache.convex) {
					if (cutPlane1 != null)
						axisFaceCache.cutAxisStrip(cutPlane1);
					if (cutPlane2 != null)
						axisFaceCache.cutAxisStrip(cutPlane2);
				} else
					axisFaceCache.cutAxisStrip(facing, cutPlane1, cutPlane2);
			} else {
				NormalPlaneF cutPlane1 = null;
				NormalPlaneF cutPlane2 = null;
				if (!faceCache.convex || (faceCache.tiltedStrip1 != null && faceCache.tiltedStrip2 != null)) {
					cutPlane1 = tiltedPlanes[j * 2];
					cutPlane2 = tiltedPlanes[j * 2 + 1];
				} else if (faceCache.tiltedStrip1 != null)
					cutPlane1 = tiltedPlanes[j * 2];
				else if (faceCache.tiltedStrip2 != null)
					cutPlane1 = tiltedPlanes[j * 2 + 1];
				
				if (faceCache.convex) {
					if (cutPlane1 != null)
						axisFaceCache.cutAxisStrip(cutPlane1);
					if (cutPlane2 != null)
						axisFaceCache.cutAxisStrip(cutPlane2);
				} else
					axisFaceCache.cutAxisStrip(facing, cutPlane1, cutPlane2);
			}
			
			if (!axisFaceCache.hasAxisStrip())
				break;
		}
	}
	
	// 将缓存存储到 SoftReference 中，以便后续使用
	this.cache = new SoftReference<>(cache);
	return cache;
}

//“轴向”在计算机图形学、数学和工程学中指的是与某个特定轴（如x轴、y轴或z轴）平行或垂直的方向。
//例如，立方体的面可以与坐标系中的某个轴（如z轴）平行，这样的面通常就被称为“轴对齐的面”或“轴向面”。