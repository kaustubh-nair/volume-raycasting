#include "polygon.h"

// PNPOLY
// https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html#The%20C%20Code
bool Polygon::point_is_inside(float x, float y)
{
    bool c = false;
    int i, j; 
    int nvert = vertices.size();
    for (i = 0, j = nvert-1; i < nvert; j = i++) {
        if ( ((vertices[i].y()>y) != (vertices[j].y()>y)) &&
            (x < (vertices[j].x()-vertices[i].x()) * (y-vertices[i].y()) / (vertices[j].y()-vertices[i].y()) + vertices[i].x()) )
        c = !c;
    }
    return c;
}
