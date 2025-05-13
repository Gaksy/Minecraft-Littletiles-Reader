% 定义网格大小
grid_type = 8;

% 定义立方体的八个顶点
dirt_box = LtBlock(3,1,3,5,3,5);
clay_1 = LtBlock(3,0,2,5,1,6);
clay_2 = LtBlock(2,0,3,3,4,5);
clay_3 = LtBlock(5,0,3,6,4,5);
clay_4 = LtBlock(3,1,2,5,4,3);
clay_5 = LtBlock(3,1,5,5,4,6);

% 绘制
showGrid(grid_type, 0);
patchLtBlock(dirt_box, 'green', 0.8);
patchLtBlock(clay_1, 'cyan', 0.8);
patchLtBlock(clay_2, 'cyan', 0.8);
patchLtBlock(clay_3, 'cyan', 0.8);
patchLtBlock(clay_4, 'cyan', 0.8);
patchLtBlock(clay_5, 'cyan', 0.8);
%cyan