/*

 * Copyright © 2018 Martino Pilia <martino.pilia@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>
#include <QVector3D>
#include <QColor>
#include <vector>

#include "mesh.h"
#include "plane.h"
#include "polygon.h"
#include "osvolume.h"

struct ColorTF {
    int id;
    QRgb rgb;
    int proximity_radius;
    float opacity;
};

/*!
 * \brief Class for a raycasting volume.
 */
class RayCastVolume : protected QOpenGLExtraFunctions
{
public:
    RayCastVolume(void);
    virtual ~RayCastVolume();

    void load_volume(const QString &filename);
    void create_noise(void);
    void paint(void);
    std::pair<double, double> range(void);


    /*!
     * \brief Get the extent of the volume.
     * \return A vector holding the extent of the bounding box.
     *
     * The extent is normalised such that the longest side of the bounding
     * box is equal to 1.
     */
    QVector3D extent(void) {
        auto e = m_scaling * m_spacing;
        return e / std::max({e.x(), e.y(), e.z()});
    }

    /*!
     * \brief Return the model matrix for the volume.
     * \param shift Shift the volume by its origin.
     * \return A matrix in homogeneous coordinates.
     *
     * The model matrix scales a two-unit side cube to the
     * extent of the volume.
     */
    QMatrix4x4 modelMatrix(bool shift = false) {
        QMatrix4x4 modelMatrix;
        if (shift) {
            modelMatrix.translate(-m_origin / scale_factor());
        }
        modelMatrix.scale(0.5f * extent());
        return modelMatrix;
    }

    /*!
     * \brief Top planes forming the AABB.
     * \param shift Shift the volume by its origin.
     * \return A vector holding the intercept of the top plane for each axis.
     */
    QVector3D top(bool shift = false) {
        auto t = extent() / 2.0;
        if (shift) {
            t -= m_origin / scale_factor();
        }
        return t;
    }

    /*!
     * \brief Bottom planes forming the AABB.
     * \param shift Shift the volume by its origin.
     * \return A vector holding the intercept of the bottom plane for each axis.
     */
    QVector3D bottom(bool shift = false) {
        auto b = -extent() / 2.0;
        if (shift) {
            b -= m_origin / scale_factor();
        }
        return b;
    }


    void updateScaling(QVector3D new_val)
    {
        m_scaling = new_val;
    }

    QVector3D getInitialSize() 
    {
        return m_size;
    }

    std::vector<int> get_initial_levels()
    {
        return std::vector<int>{volume->_curr_level, volume->levels-1};
    }

    void switch_to_low_res()
    {
        volume->switch_to_low_res();
        update_volume_texture();
    }

    int load_best_res()
    {
        int level = volume->load_best_res();

        update_volume_texture();

        return level;
    }

    void zoom_in()
    {
        volume->switch_to_low_res();
        volume->zoom_in();
        update_volume_texture();
    }

    void zoom_out()
    {
        volume->switch_to_low_res();
        volume->zoom_out();
        update_volume_texture();
    }

    void move_left()
    {
        volume->switch_to_low_res();
        volume->move_left();
        update_volume_texture();
    }
    void move_up()
    {
        volume->switch_to_low_res();
        volume->move_up();
        update_volume_texture();
    }
    void move_down()
    {
        volume->switch_to_low_res();
        volume->move_down();
        update_volume_texture();
    }
    void move_right()
    {
        volume->switch_to_low_res();
        volume->move_right();
        update_volume_texture();
    }

    void enable_lighting(bool value)
    {
        lighting_enabled = value;
    }
    void set_vram(int value){volume->set_vram(value);}

    void initialize_color_proximity_tf();
    void set_color_proximity_tf_data(QRgb rgb, int id);
    void update_color_proximity_tf_data();
    void update_color_proximity_tf_opacity(int id, int opacity);
    void update_color_proximity_tf_size(int id, int size);

    void update_segment_opacity(int id, int opacity);
    void update_volume_opacity(int opacity);

    float tf_threshold = 1.0;
    float hsv_tf_h_threshold = 1.0;
    float hsv_tf_s_threshold = 1.0;
    float hsv_tf_v_threshold = 1.0;
    bool lighting_enabled = false;

    void update_location_tf();
    void update_location_proximity_tf_opacity(int id, int opacity);
    void update_slicing_plane_opacity(int id, int opacity);
    void update_slicing_plane_orientation(int id, int value);
    void update_slicing_plane_distance(int id, int value);

    void add_new_slicing_plane(int id) { slicing_planes.push_back(Plane(id)); update_location_tf();}

    std::vector<Polygon> polygons;
    std::vector<Plane> slicing_planes;

private:
    const static int MAX_NUM_SEGMENTS = 3;
    const static int LOCATION_TF_DIMENSION = 256;
    const static int COLOR_TF_DIMENSION = 256;
    GLuint m_volume_texture;
    GLuint m_noise_texture;
    GLuint m_tf_texture;
    GLuint m_location_tf_texture;
    GLuint m_segment_opacity_texture;
    Mesh m_cube_vao;
    std::pair<double, double> m_range;
    QVector3D m_origin;
    QVector3D m_spacing;
    QVector3D m_size;
    QVector3D m_scaling;
    float volume_opacity = 1.0;


    OSVolume *volume;

    float color_proximity_tf[COLOR_TF_DIMENSION][COLOR_TF_DIMENSION][COLOR_TF_DIMENSION];
    float location_tf[LOCATION_TF_DIMENSION][LOCATION_TF_DIMENSION][LOCATION_TF_DIMENSION];
    float segment_opacity_tf[MAX_NUM_SEGMENTS];
    float COLOR_PROX_TF_DEFAULT_RADIUS = 1;
    float SPACE_PROX_TF_DEFAULT_RADIUS = 100;
    int j = 0;

    float scale_factor(void);
    uint32_t rgb(int x, int y, int z, int size);
    void initialize_texture_data();
    void update_segment_opacity_texture();
    void update_volume_texture();
    void update_location_tf_texture();
    void update_location_tf_data();
    void update_color_prox_texture();
    std::vector<ColorTF> color_tf_data;

};
