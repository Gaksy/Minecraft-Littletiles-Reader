% 定义网格大小
grid_type = 4;

% 定义立方体的八个顶点
block_aabb = LtBlock(0,0,0,4,4,4);
block = LtBlock(1,0,1,3,1,2);
block.applyOffset(AngleID.WDS, LtPoint(0, 0, 0));
block.applyOffset(AngleID.WDN, LtPoint(0, 0, 0));
block.applyOffset(AngleID.WUS, LtPoint(0, 0, 0));
block.applyOffset(AngleID.WUN, LtPoint(0, 0, 0));
block.applyOffset(AngleID.EDS, LtPoint(0, 0, 0));
block.applyOffset(AngleID.EDN, LtPoint(0, 0, 0));
block.applyOffset(AngleID.EUS, LtPoint(0, -1, 0));
block.applyOffset(AngleID.EUN, LtPoint(0, -1, 0));
% 绘制
showGrid(grid_type, -2, 2);
patchLtBlock(block, 'green', 0.7);
patchLtBlock(block_aabb, 'blue', 0.1);

%cyan