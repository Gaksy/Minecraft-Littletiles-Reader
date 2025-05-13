classdef LtBlock < handle
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
            arguments
                obj LtBlock
            end
            vertices = [
                obj.WDN.x obj.WDN.z obj.WDN.y;
                obj.EDN.x obj.EDN.z obj.EDN.y;
                obj.EDS.x obj.EDS.z obj.EDS.y;
                obj.WDS.x obj.WDS.z obj.WDS.y;  % 这里应该是 obj.WDS.y?
                obj.WUN.x obj.WUN.z obj.WUN.y;
                obj.EUN.x obj.EUN.z obj.EUN.y;
                obj.EUS.x obj.EUS.z obj.EUS.y;
                obj.WUS.x obj.WUS.z obj.WUS.y;
            ];
        end

        function applyOffset(obj, angle_id, offset_data)
            arguments
                obj LtBlock
                angle_id AngleID
                offset_data LtPoint
            end
            
            switch angle_id
                case AngleID.EUN
                    obj.EUN.x = obj.EUN.x + offset_data.x;
                    obj.EUN.y = obj.EUN.y + offset_data.y;
                    obj.EUN.z = obj.EUN.z + offset_data.z;
                    
                case AngleID.EUS
                    obj.EUS.x = obj.EUS.x + offset_data.x;
                    obj.EUS.y = obj.EUS.y + offset_data.y;
                    obj.EUS.z = obj.EUS.z + offset_data.z;
                    
                case AngleID.EDN
                    obj.EDN.x = obj.EDN.x + offset_data.x;
                    obj.EDN.y = obj.EDN.y + offset_data.y;
                    obj.EDN.z = obj.EDN.z + offset_data.z;
                    
                case AngleID.EDS
                    obj.EDS.x = obj.EDS.x + offset_data.x;
                    obj.EDS.y = obj.EDS.y + offset_data.y;
                    obj.EDS.z = obj.EDS.z + offset_data.z;
                    
                case AngleID.WUN
                    obj.WUN.x = obj.WUN.x + offset_data.x;
                    obj.WUN.y = obj.WUN.y + offset_data.y;
                    obj.WUN.z = obj.WUN.z + offset_data.z;
                    
                case AngleID.WUS
                    obj.WUS.x = obj.WUS.x + offset_data.x;
                    obj.WUS.y = obj.WUS.y + offset_data.y;
                    obj.WUS.z = obj.WUS.z + offset_data.z;
                    
                case AngleID.WDN
                    obj.WDN.x = obj.WDN.x + offset_data.x;
                    obj.WDN.y = obj.WDN.y + offset_data.y;
                    obj.WDN.z = obj.WDN.z + offset_data.z;
                    
                case AngleID.WDS
                    obj.WDS.x = obj.WDS.x + offset_data.x;
                    obj.WDS.y = obj.WDS.y + offset_data.y;
                    obj.WDS.z = obj.WDS.z + offset_data.z;
            end
        end
    end
end