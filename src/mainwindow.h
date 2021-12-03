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

#include <QMainWindow>
#include<QGridLayout>
#include "my_q_slider.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow *ui;
    void mousePressEvent(QMouseEvent *event);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

private slots:

    void load_volume(const QString& path);

    void on_stepLength_valueChanged(double arg1);

    void on_loadVolume_clicked();

    void on_background_clicked();

    void on_height_spinbox_valueChanged();

    void on_vram_spinbox_valueChanged();

    void on_width_spinbox_valueChanged();

    void on_depth_spinbox_valueChanged();

    void on_best_res_button_clicked();

    void on_zoom_in_button_clicked();

    void on_zoom_out_button_clicked();

    void on_up_button_clicked();

    void on_down_button_clicked();

    void on_left_button_clicked();

    void on_right_button_clicked();

    void on_add_slicing_plane_button_clicked();

    void on_segment_1_opacity_valueChanged(int value);

    void on_segment_2_opacity_valueChanged(int value);

    void on_segment_3_opacity_valueChanged(int value);

    void on_enable_lighting_checkbox_clicked(bool value);

    void on_volume_opacity_slider_valueChanged(int value);

    void on_light_x_position_valueChanged(int value);
    void on_light_y_position_valueChanged(int value);
    void on_light_z_position_valueChanged(int value);




private:
    std::string curr_level_label, max_level_label;
    int i = 0;
    QGridLayout *prox_scroll_layout = nullptr;
    QWidget *prox_scroll_layout_main = nullptr;
    int color_tf_slider_count = 0;    // number of color tfs - used to assign slider ids
    int location_tf_slider_count = 0;    // number of location tfs - used to assign slider ids
    int slicing_planes_count = 0;    // number of slicing planes - used to assigne ids
};
