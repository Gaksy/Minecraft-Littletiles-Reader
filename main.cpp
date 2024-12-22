#include <cstdio>
#include <Galib/Coord/Coord2D.h>

using galib::coord::Coordinate2D;

int main() {
    Coordinate2D<int> coordinate_2d{0, 0};
    GALIB_STD printf("%d, %d\n", coordinate_2d.x, coordinate_2d.z);
    return 0;
}
