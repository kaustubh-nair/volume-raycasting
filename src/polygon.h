
#pragma once

#include<cstdio>
#include<vector>
#include<QVector3D>

// Polygons for location based TF
// location stored relative to model space of volume
// currently only support for polygons parallel to volume
class Polygon
{
    public:
        void add_point(int ID, float x, float y, float z){
            vertices.push_back(QVector3D(x,y,z));
            id = ID;
        }
        void set_opacity(float o){opacity = o;};
        float get_opacity(){return opacity;}
        bool point_is_inside(float x, float y);
        int id;
    private:
        float opacity = 0.0f;
        std::vector<QVector3D> vertices;
        bool enabled = true;

};
