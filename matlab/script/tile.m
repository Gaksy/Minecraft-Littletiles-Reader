% 定义网格大小
grid_type = 4;

% 定义立方体的八个顶点
EUN = LtPoint(2, 2, 0); %
EUS = LtPoint(2, 0, 2); %
EDN = LtPoint(2, 0, 0); %
EDS = LtPoint(2, 0, 2); %
WUN = LtPoint(0, 2, 0); %
WUS = LtPoint(0, 0, 2); %
WDN = LtPoint(0, 0, 0); %
WDS = LtPoint(0, 0, 2); %
vertices = getLtCubVertices(EUN, EUS, EDN, EDS, WUN, WUS, WDN, WDS);

% 绘制
showGrid(grid_type);
patch('Vertices', vertices, 'Faces', getLtFaces(), 'FaceColor', 'cyan', 'FaceAlpha', 0.8);

