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
    ui->tf_slider->setDisabled(true);

    // Populate list of visualisation modes
    for (const auto& mode : ui->canvas->getModes()) {
        ui->mode->addItem(mode);
    }
    ui->mode->setCurrentIndex(0);

    // Enable file drop
    setAcceptDrops(true);
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

/*!
 * \brief Load a volume from file.
 */
void MainWindow::on_loadVolume_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open volume"), ".", tr("Images (*.vtk *.tiff *.svs *.tif)"));
    if (!path.isNull()) {
        load_volume(path);
    }
}

void MainWindow::on_tf_slider_valueChanged(int value)
{
    ui->canvas->setTFThreshold(value);
}

void MainWindow::on_tf_checkbox_clicked(bool value)
{
    if (value)
    {
        ui->tf_slider->setEnabled(true);
        ui->canvas->setTFThreshold(ui->tf_slider->value());
    }
    else
    {
        ui->tf_slider->setDisabled(true);
        ui->canvas->setTFThreshold(100);

    }
}

void MainWindow::on_hsv_tf_checkbox_clicked(bool value)
{
    if (value)
    {
        ui->HSV_TF_h_slider->setEnabled(true);
        ui->HSV_TF_s_slider->setEnabled(true);
        ui->HSV_TF_v_slider->setEnabled(true);
        ui->canvas->setHSV_TF_HThreshold(ui->HSV_TF_h_slider->value());
        ui->canvas->setHSV_TF_SThreshold(ui->HSV_TF_s_slider->value());
        ui->canvas->setHSV_TF_VThreshold(ui->HSV_TF_v_slider->value());
    }
    else
    {
        ui->HSV_TF_h_slider->setDisabled(true);
        ui->HSV_TF_s_slider->setDisabled(true);
        ui->HSV_TF_v_slider->setDisabled(true);
        ui->canvas->setHSV_TF_HThreshold(100);
        ui->canvas->setHSV_TF_SThreshold(100);
        ui->canvas->setHSV_TF_VThreshold(100);

    }
}


void MainWindow::on_HSV_TF_h_slider_valueChanged(int value)
{
    ui->canvas->setHSV_TF_HThreshold(value);

}

void MainWindow::on_HSV_TF_s_slider_valueChanged(int value)
{
    ui->canvas->setHSV_TF_SThreshold(value);

}

void MainWindow::on_HSV_TF_v_slider_valueChanged(int value)
{
    ui->canvas->setHSV_TF_VThreshold(value);
}


/*!
 * \brief Set the visualisation mode.
 * \param arg1 Name of the visualisation mode.
 */
void MainWindow::on_mode_currentTextChanged(const QString &mode)
{
    ui->canvas->setMode(mode);
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
