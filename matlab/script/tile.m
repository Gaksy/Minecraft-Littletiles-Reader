% 定义网格大小
grid_type = 4;

% 定义立方体的八个顶点
block = LtBlock(1,1,1,3,3,3);
block.EUS.y = 1;
block.EUN.y = 1;
% 绘制
showGrid(grid_type, 0);
patchLtBlock(block, 'green', 0.7);

%cyan