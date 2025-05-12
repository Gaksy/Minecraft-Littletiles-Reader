function showGrid(grid_type)
    figure;
    axis([0 grid_type 0 grid_type 0 grid_type]);     % 设置坐标轴范围更大一些以显示1,2,3
    axis equal;
    grid on;
    
    xlabel('X (+x East)');               % X 轴不变
    ylabel('Z (+z South)');               % 原 Y 轴，标签设为 Z
    zlabel('Y (+y Up)');               % 原 Z 轴，标签设为 Y
    
    view([1 1 1]);             % 设置一个等角视图
    
    % 反转X轴（现在作为Z使用）
    set(gca, 'XDir', 'reverse');
    
    % 设置刻度为1的倍数
    xticks(-1:1:4);   % 设置 x 轴刻度
    yticks(-1:1:4);   % 设置 y 轴刻度
    zticks(-1:1:4);   % 设置 z 轴刻度（如果是 3D 图）
    xlim([-1 4]);
    ylim([-1 4]);
    zlim([-1 4]);   % 同样适用于 3D 图

end