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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QSignalBlocker>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow {parent}
    , ui {new Ui::MainWindow}
{
    ui->setupUi(this);

    // Set inital values
    ui->stepLength->valueChanged(ui->stepLength->value());
    ui->canvas->setBackground(Qt::black);

    // Enable file drop
    setAcceptDrops(true);

    // initialize scroll area for TFs
    QScrollArea *scroll = ui->scrollArea;
    prox_scroll_layout_main = new QWidget;
    prox_scroll_layout = new QGridLayout(prox_scroll_layout_main);

    scroll->setWidget(prox_scroll_layout_main);
    scroll->setWidgetResizable(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}


/*!
 * \brief Allow dragged files to enter the main window.
 * \param event Drag enter event.
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}


/*!
 * \brief Handle drop events to load volumes.
 * \param event Drop event.
 */
void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        for (auto& url : mimeData->urls()) {
            load_volume(url.toLocalFile());
        }
    }
}


/*!
 * \brief Load a volume
 * \param path Volume file to be loaded.
 *
 * Try to load the volume. Update the UI if succesfull, or prompt an error
 * message in case of failure.
 */
void MainWindow::load_volume(const QString& path)
{
    try {
        ui->canvas->setVolume(path);

        // set scaling spinboxes
        QVector3D size = ui->canvas->getInitialSize();
        ui->height_spinbox->setValue(size.x());
        ui->width_spinbox->setValue(size.y());
        ui->depth_spinbox->setValue(size.z());

        std::vector<int> levels = ui->canvas->get_initial_levels();
        curr_level_label = std::to_string(levels[0]+1);
        max_level_label =  std::to_string(levels[1]+1);

        ui->num_levels_label->setText((curr_level_label + "/" + max_level_label).c_str());

    }
    catch (std::runtime_error& e) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot load volume ") + path + ": " + e.what());
    }
}

/*!
 * \brief Set the ray marching step lenght.
 * \param arg1 Step length, as a fraction of the ray length.
 */
void MainWindow::on_stepLength_valueChanged(double arg1)
{
    ui->canvas->setStepLength(static_cast<GLfloat>(arg1));
}

void MainWindow::on_zoom_in_button_clicked()
{
    ui->canvas->zoom_in();

}

void MainWindow::on_zoom_out_button_clicked()
{
    ui->canvas->zoom_out();
}
void MainWindow::on_up_button_clicked()
{
    ui->canvas->move_up();
}
void MainWindow::on_down_button_clicked()
{
    ui->canvas->move_down();
}
void MainWindow::on_left_button_clicked()
{
    ui->canvas->move_left();
}
void MainWindow::on_right_button_clicked()
{
    ui->canvas->move_right();
}

/* 
 * set best possible resolution
 */
void MainWindow::on_best_res_button_clicked()
{
    int l = ui->canvas->load_best_res()+1;
    curr_level_label = std::to_string(l);
    ui->num_levels_label->setText((curr_level_label + "/" + max_level_label).c_str());
}

/*!
 * \brief Load a volume from file.
 */
void MainWindow::on_loadVolume_clicked()
{

    // hardcode for quick testing
    std::string str = "../cmu_preprocessed_pyramidal.tiff";
    QString path = QString::fromStdString(str);

    // QString path = QFileDialog::getOpenFileName(this, tr("Open volume"), ".", tr("Images (*.vtk *.tiff *.svs *.tif)"));
    if (!path.isNull()) {
        load_volume(path);
    }
}

void MainWindow::on_vram_spinbox_valueChanged()
{
    ui->canvas->set_vram(ui->vram_spinbox->value());
}

void MainWindow::on_height_spinbox_valueChanged()
{
    ui->canvas->updateScaling(QVector3D(ui->height_spinbox->value(), ui->width_spinbox->value(), ui->depth_spinbox->value()));
}

void MainWindow::on_width_spinbox_valueChanged()
{
    ui->canvas->updateScaling(QVector3D(ui->height_spinbox->value(), ui->width_spinbox->value(), ui->depth_spinbox->value()));
}

void MainWindow::on_depth_spinbox_valueChanged()
{
    ui->canvas->updateScaling(QVector3D(ui->height_spinbox->value(), ui->width_spinbox->value(), ui->depth_spinbox->value()));
}

void MainWindow::on_segment_1_opacity_valueChanged(int value)
{
    ui->canvas->update_segment_opacity(0, value);

}

void MainWindow::on_segment_2_opacity_valueChanged(int value)
{
    ui->canvas->update_segment_opacity(1, value);

}

void MainWindow::on_segment_3_opacity_valueChanged(int value)
{
    ui->canvas->update_segment_opacity(2, value);

}

void MainWindow::on_enable_lighting_checkbox_clicked(bool value)
{
    ui->canvas->enable_lighting(value);
}

/*!
 * \brief Open a dialog to choose the background colour.
 */
void MainWindow::on_background_clicked()
{
    const QColor colour = QColorDialog::getColor(ui->canvas->getBackground(), this, "Select background colour");

    if (colour.isValid()) {
        ui->canvas->setBackground(colour);
    }
}
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // use the frambuffer instead of glReadPixels due to some weird qt widget scaling
    QCheckBox *color_checkbox = ui->proximity_color_tf_checkbox;
    QCheckBox *space_checkbox = ui->proximity_space_tf;
    if (color_checkbox->isChecked())
    {
        int color_tf_id = color_tf_slider_count++;
        QImage image = ui->canvas->grabFramebuffer();
        QPointF pos = event->windowPos();
        QRgb rgb = image.pixel(pos.x(), pos.y());
        ui->canvas->set_color_proximity_tf(rgb, color_tf_id);


        // Add sliders and color indicator to the UI
        QScrollArea *scroll = ui->scrollArea;

        QLabel *color_label = new QLabel;
        std::string str = "background:rgb(" + std::to_string(qRed(rgb))+ "," + std::to_string( qGreen(rgb)) + ","+std::to_string( qBlue(rgb))+");";
        color_label->setStyleSheet(QString::fromStdString(str));
        int rows = prox_scroll_layout->rowCount();
        QWidget *c = new QWidget;
        QGridLayout *l = new QGridLayout(c);

        MyQSlider *opacity_bar = new MyQSlider(Qt::Horizontal);
        MyQSlider *size_bar = new MyQSlider(Qt::Horizontal);
        const QString name = QString::fromStdString("opacity_bar_" + std::to_string(color_tf_id));
        const QString c_name = QString::fromStdString("color_bar_" + std::to_string(color_tf_id));
        opacity_bar->setObjectName(name);
        size_bar->setObjectName(c_name);

        connect(opacity_bar, &MyQSlider::valueChanged, opacity_bar, &MyQSlider::myValueChanged);
        connect(opacity_bar, &MyQSlider::myValueChangedWithId, ui->canvas, &RayCastCanvas::update_color_tf_opacity);

        connect(size_bar, &MyQSlider::valueChanged, size_bar, &MyQSlider::myValueChanged);
        connect(size_bar, &MyQSlider::myValueChangedWithId, ui->canvas, &RayCastCanvas::update_color_tf_size);

        l->addWidget(opacity_bar,0,0);
        l->addWidget(size_bar,1,0);
        prox_scroll_layout->addWidget(color_label,rows,0);
        prox_scroll_layout->addWidget(c,rows,1);
        scroll->setWidget(prox_scroll_layout_main);
    }
    else if(space_checkbox->isChecked())
    {
        int location_tf_id = location_tf_slider_count;
        QPointF pos = event->windowPos();
        ui->canvas->set_space_proximity_tf(location_tf_id, pos.x(), pos.y(), event->buttons() & Qt::LeftButton, event->buttons() & Qt::RightButton);

        if (Qt::RightButton & event->buttons())
        {
            int rows = prox_scroll_layout->rowCount();
            QWidget *c = new QWidget;
            QGridLayout *l = new QGridLayout(c);
            QScrollArea *scroll = ui->scrollArea;
            MyQSlider *opacity_bar = new MyQSlider(Qt::Horizontal);
            const QString name = QString::fromStdString("opacity_bar_" + std::to_string(location_tf_id));
            printf("location tf id %d\n", location_tf_id);
            opacity_bar->setObjectName(name);
            connect(opacity_bar, &MyQSlider::valueChanged, opacity_bar, &MyQSlider::myValueChanged);
            connect(opacity_bar, &MyQSlider::myValueChangedWithId, ui->canvas, &RayCastCanvas::update_location_tf_opacity);
            l->addWidget(opacity_bar,0,0);

            QLabel *label = new QLabel("Poly");
            prox_scroll_layout->addWidget(label,rows,0);
            prox_scroll_layout->addWidget(c,rows,1);
            location_tf_slider_count++;
        }
    }
}

void MainWindow::on_volume_opacity_slider_valueChanged(int value)
{
    ui->canvas->update_volume_opacity(value);
}

void MainWindow::on_light_x_position_valueChanged(int value)
{
    ui->canvas->update_light_position_x(value);
}

void MainWindow::on_light_y_position_valueChanged(int value)
{
    ui->canvas->update_light_position_y(value);
}

void MainWindow::on_light_z_position_valueChanged(int value)
{
    ui->canvas->update_light_position_z(value);
}
