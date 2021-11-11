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

#include <functional>
#include <vector>

#include <QtMath>

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

#include "mesh.h"
#include "polygon.h"
#include "raycastvolume.h"
#include "trackball.h"
#include "vtkvolume.h"

/*!
 * \brief Class for a raycasting canvas widget.
 */
class RayCastCanvas : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:

    explicit RayCastCanvas(QWidget *parent = nullptr);
    ~RayCastCanvas();

    void setStepLength(const GLfloat step_length) {
        m_stepLength = step_length;
        update();
    }

    void setVolume(const QString& volume) {
        m_raycasting_volume->load_volume(volume);
        update();
    }

    void setThreshold(const double threshold) {
        auto range = m_raycasting_volume ? getRange() : std::pair<double, double>{0.0, 1.0};
        m_threshold = threshold / (range.second - range.first);
        update();
    }

    void setMode(const QString& mode) {
        m_active_mode = mode;
        update();
    }

    void setBackground(const QColor& colour) {
        m_background = colour;
        update();
    }

    void setTFThreshold(const float t)
    {
        m_raycasting_volume->tf_threshold = t/100.0;
        update();
    }

    void setHSV_TF_HThreshold(const float t)
    {
        m_raycasting_volume->hsv_tf_h_threshold = t/100.0;
        update();
    }

    void setHSV_TF_SThreshold(const float t)
    {
        m_raycasting_volume->hsv_tf_s_threshold = t/100.0;
        update();
    }

    void setHSV_TF_VThreshold(const float t)
    {
        m_raycasting_volume->hsv_tf_v_threshold = t/100.0;
        update();
    }

    void updateScaling(QVector3D inc)
    {
        m_raycasting_volume->updateScaling(inc);
        update();
    }

    std::vector<QString> getModes(void) {
        std::vector<QString> modes;
        for (const auto& [key, val] : m_modes) {
            modes.push_back(key);
        }
        return modes;
    }

    QColor getBackground(void) {
        return m_background;
    }

    std::pair<double, double> getRange(void) {
        return m_raycasting_volume->range();
    }

    QVector3D getInitialSize() {
        return m_raycasting_volume->getInitialSize();
    }

    std::vector<int> get_initial_levels() {
        return m_raycasting_volume->get_initial_levels();
    }

    // returns current level
    int load_best_res()
    {
        int l = m_raycasting_volume->load_best_res();
        update();
        return l;

    }

    void move_up()
    {
        m_raycasting_volume->move_up();
        update();
    }
    void move_left()
    {
        m_raycasting_volume->move_left();
        update();
    }
    void move_down()
    {
        m_raycasting_volume->move_down();
        update();
    }
    void move_right()
    {
        m_raycasting_volume->move_right();
        update();
    }

    void zoom_out()
    {
        m_raycasting_volume->zoom_out();
        update();
    }

    void zoom_in()
    {
        m_raycasting_volume->zoom_in();
        update();
    }

    void set_space_proximity_tf(qreal x, qreal y, bool left_mouse_pressed, bool right_mouse_pressed)
    {
        if (left_mouse_pressed)
            location_tf_add_side_to_polygon(x, y);
        else if(right_mouse_pressed)
            location_tf_close_current_polygon(x, y);
        update();

    }
    void set_color_proximity_tf(QRgb rgb)
    {
        m_raycasting_volume->set_color_proximity_tf(rgb);
        update();
    }

    void update_segment_opacity(int id, int opacity)
    {
        m_raycasting_volume->update_segment_opacity(id, opacity);
        update();
    }

    void enable_lighting(bool value)
    {
        m_raycasting_volume->enable_lighting(value);
        update();
    }

    void set_vram(int value) { m_raycasting_volume->set_vram(value); }


signals:
    // NOPE

public slots:
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent * event);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:

    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelViewProjectionMatrix;
    QMatrix3x3 m_normalMatrix;

    const GLfloat m_fov = 60.0f;                                          /*!< Vertical field of view. */
    const GLfloat m_focalLength = 1.0 / qTan(M_PI / 180.0 * m_fov / 2.0); /*!< Focal length. */
    GLfloat m_aspectRatio;                                                /*!< width / height */

    QVector2D m_viewportSize;
    QVector3D m_rayOrigin; /*!< Camera position in model space coordinates. */

    QVector3D m_lightPosition {3.0, 0.0, 3.0};    /*!< In camera coordinates. */
    QVector3D m_diffuseMaterial {1.0, 1.0, 1.0};  /*!< Material colour. */
    GLfloat m_stepLength;                         /*!< Step length for ray march. */
    GLfloat m_threshold;                          /*!< Isosurface intensity threshold. */
    QColor m_background;                          /*!< Viewport background colour. */


    const GLfloat m_gamma = 2.2f; /*!< Gamma correction parameter. */

    RayCastVolume *m_raycasting_volume;

    std::map<QString, QOpenGLShaderProgram*> m_shaders;
    std::map<QString, std::function<void(void)>> m_modes;
    QString m_active_mode;

    TrackBall m_trackBall {};       /*!< Trackball holding the model rotation. */
    TrackBall m_scene_trackBall {}; /*!< Trackball holding the scene rotation. */

    GLint m_distExp = -200;

    GLuint scaled_width();
    GLuint scaled_height();

    void raycasting(const QString& shader);

    QPointF pixel_pos_to_view_pos(const QPointF& p);
    void create_noise(void);
    void add_shader(const QString& name, const QString& vector, const QString& fragment);

    // location/polygon TF related data
    bool polygon_creation_active = false;
    void location_tf_close_current_polygon(qreal x, qreal y);
    void location_tf_add_side_to_polygon(qreal x, qreal y);
    std::vector<Polygon> polygons;


};
