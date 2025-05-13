% 定义网格大小
grid_type = 2;

% 定义立方体的八个顶点
block_aabb = LtBlock(0,0,0,2,2,2);
block = LtBlock(0,0,0,2,1,1);
%block = LtBlock(2,2,2,4,3,3);
block.applyOffset(AngleID.WDS, LtPoint(-2, 0, 2));
block.applyOffset(AngleID.WDN, LtPoint(2, 0, -2));
block.applyOffset(AngleID.WUS, LtPoint(-2, 0, 2));
block.applyOffset(AngleID.WUN, LtPoint(2, 0, -2));
block.applyOffset(AngleID.EDS, LtPoint(-2, 0, 2));
block.applyOffset(AngleID.EDN, LtPoint(2, 0, -2));
block.applyOffset(AngleID.EUS, LtPoint(-2, 0, 2));
block.applyOffset(AngleID.EUN, LtPoint(2, 0, -2));
% 绘制
showGrid(grid_type, -2, 2);
patchLtBlock(block, 'green', 0.7);
patchLtBlock(block_aabb, 'blue', 0.1);

%cyan