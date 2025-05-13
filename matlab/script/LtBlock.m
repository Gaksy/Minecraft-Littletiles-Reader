classdef LtBlock
    properties
       EUN LtPoint
       EUS LtPoint
       EDN LtPoint
       EDS LtPoint
       WUN LtPoint
       WUS LtPoint
       WDN LtPoint
       WDS LtPoint
    end
    methods
        function obj = LtBlock(x_1, y_1, z_1, x_2, y_2, z_2)
            obj.WDS = LtPoint(x_1, y_1, z_2);
            obj.WDN = LtPoint(x_1, y_1, z_1);
            obj.EDN = LtPoint(x_2, y_1, z_1);
            obj.EDS = LtPoint(x_2, y_1, z_2);
            obj.WUN = LtPoint(x_1, y_2, z_1);
            obj.WUS = LtPoint(x_1, y_2, z_2);
            obj.EUS = LtPoint(x_2, y_2, z_2);
            obj.EUN = LtPoint(x_2, y_2, z_1);
        end

        function vertices = getLtCubVertices(obj)
            vertices = [
                obj.WDN.x obj.WDN.z obj.WDN.y;
                obj.EDN.x obj.EDN.z obj.EDN.y;
                obj.EDS.x obj.EDS.z obj.EDS.y;
                obj.WDS.x obj.WDS.z obj.WDN.y;
                obj.WUN.x obj.WUN.z obj.WUN.y;
                obj.EUN.x obj.EUN.z obj.EUN.y;
                obj.EUS.x obj.EUS.z obj.EUS.y;
                obj.WUS.x obj.WUS.z obj.WUS.y;
            ];
        end
    end
end