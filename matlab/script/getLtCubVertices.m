function vertices = getLtCubVertices(EUN, EUS, EDN, EDS, WUN, WUS, WDN, WDS)
    arguments
        EUN LtPoint
        EUS LtPoint
        EDN LtPoint
        EDS LtPoint
        WUN LtPoint
        WUS LtPoint
        WDN LtPoint
        WDS LtPoint
    end
    % (x is x, y is z, z is y)
    vertices = [
        WDN.x WDN.z WDN.y;
        EDN.x EDN.z EDN.y;
        EDS.x EDS.z EDS.y;
        WDS.x WDS.z WDN.y;
        WUN.x WUN.z WUN.y;
        EUN.x EUN.z EUN.y;
        EUS.x EUS.z EUS.y;
        WUS.x WUS.z WUS.y;
    ]
end