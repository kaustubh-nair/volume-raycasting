#include "plane.h"

bool Plane::point_is_inside(float x, float y, float z)
{
    bool left = true;
    if (orientation == 0)
    {
        if (x > distance)
            left = false;
    }
    else if (orientation == 1)
    {
        if (y > distance)
            left = false;

    }
    else if (orientation == 2)
    {
        if (z > distance)
            left = false;

    }
    if (hide_left)
        return left;
    else
        return !left;
}
