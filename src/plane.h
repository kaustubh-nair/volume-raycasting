
#pragma once

#include<cstdio>
#include<vector>

class Plane {
    public:
    int id;
    float opacity;

    Plane(int Id)
    {
        id = Id;
        orientation = 0;
        opacity = 0.0;
        distance = 0.1;
        hide_left = true;
    }
    void invert(){hide_left = !hide_left;}
    void update_orientation(int o) { orientation = o;; }
    void update_distance(float d) {distance = d;}
    bool point_is_inside(float x, float y, float z);

    private:
    int orientation; // 0,1,2 for x,y,z respectively
    float distance;

    bool hide_left; // used to invert direction that slicing plane hides stuff in
};
