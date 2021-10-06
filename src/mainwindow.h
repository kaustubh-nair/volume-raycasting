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

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

private slots:

    void load_volume(const QString& path);

    void on_stepLength_valueChanged(double arg1);

    void on_loadVolume_clicked();

    void on_tf_slider_valueChanged(int value);

    void on_background_clicked();

    void on_tf_checkbox_clicked(bool value);

    void on_HSV_TF_h_slider_valueChanged(int value);

    void on_HSV_TF_s_slider_valueChanged(int value);

    void on_HSV_TF_v_slider_valueChanged(int value);

    void on_hsv_tf_checkbox_clicked(bool value);

    void on_height_spinbox_valueChanged();

    void on_width_spinbox_valueChanged();

    void on_depth_spinbox_valueChanged();

    void on_best_res_button_clicked();

private:
    Ui::MainWindow *ui;
    std::string curr_level_label, max_level_label;
};
