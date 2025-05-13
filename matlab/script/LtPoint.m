classdef LtPoint
    % 三个轴向的坐标点
    properties
       x
       y
       z
    end

    methods
        function obj = LtPoint(x, y, z)
            obj.x = x;
            obj.y = y;
            obj.z = z;
        end
    end

end