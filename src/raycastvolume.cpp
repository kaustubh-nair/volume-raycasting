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


#include "raycastvolume.h"
#include "vtkvolume.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>


float eucl_dist(int a, int b, int c, int x, int y, int z);

/*!
 * \brief Create a two-unit cube mesh as the bounding box for the volume.
 */
RayCastVolume::RayCastVolume(void)
    : m_volume_texture {0}
    , m_noise_texture {0}
    , m_tf_texture {0}
    , m_cube_vao {
          {
              -1.0f, -1.0f,  1.0f,
               1.0f, -1.0f,  1.0f,
               1.0f,  1.0f,  1.0f,
              -1.0f,  1.0f,  1.0f,
              -1.0f, -1.0f, -1.0f,
               1.0f, -1.0f, -1.0f,
               1.0f,  1.0f, -1.0f,
              -1.0f,  1.0f, -1.0f,
          },
          {
              // front
              0, 1, 2,
              0, 2, 3,
              // right
              1, 5, 6,
              1, 6, 2,
              // back
              5, 4, 7,
              5, 7, 6,
              // left
              4, 0, 3,
              4, 3, 7,
              // top
              2, 6, 7,
              2, 7, 3,
              // bottom
              4, 5, 1,
              4, 1, 0,
          }
      }
{
    initializeOpenGLFunctions();
}


/*!
 * \brief Destructor.
 */
RayCastVolume::~RayCastVolume()
{
}


/*!
 * \brief Load a volume from file.
 * \param File to be loaded.
 */
void RayCastVolume::load_volume(const QString& filename) {

    QRegularExpression re {"^.*\\.([^\\.]+)$"};
    QRegularExpressionMatch match = re.match(filename);

    if (!match.hasMatch()) {
        throw std::runtime_error("Cannot determine file extension.");
    }

    const std::string extension {match.captured(1).toLower().toStdString()};
    if ("vtk" == extension) {
        std::vector<unsigned char> data;
        VTKVolume volume {filename.toStdString()};
        volume.uint8_normalised();
        m_size = QVector3D(std::get<0>(volume.size()), std::get<1>(volume.size()), std::get<2>(volume.size()));
        m_origin = QVector3D(std::get<0>(volume.origin()), std::get<1>(volume.origin()), std::get<2>(volume.origin()));
        m_spacing = QVector3D(std::get<0>(volume.spacing()), std::get<1>(volume.spacing()), std::get<2>(volume.spacing()));
        m_range = volume.range();
        data = volume.data();

        glDeleteTextures(1, &m_volume_texture);
        glGenTextures(1, &m_volume_texture);
        glBindTexture(GL_TEXTURE_3D, m_volume_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // The array on the host has 1 byte alignment
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, m_size.x(), m_size.y(), m_size.z(), 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    else if ("tiff" == extension || "svs" == extension || "tif" == extension) {
        uint32_t* data;
        volume  = new OSVolume({filename.toStdString()});

        data = volume->data();
        m_spacing = QVector3D(0.5f,0.5f, 0.5f);
        m_origin = QVector3D(0.0f, 0.0f, 0.0f);
        m_size = volume->size();
        m_scaling = m_size;

        glDeleteTextures(1, &m_volume_texture);
        glGenTextures(1, &m_volume_texture);
        glBindTexture(GL_TEXTURE_3D, m_volume_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, m_size.x(),m_size.y(),m_size.z(),0,GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        glGenerateMipmap(GL_TEXTURE_3D);
        glBindTexture(GL_TEXTURE_3D, 0);



        for(int i = 0; i < 256; i++)
        {
            for(int j = 0; j < 256; j++)
            {
                for(int k = 0; k < 256; k++)
                {
                    color_proximity_tf[i][j][k] = 1.0f;
                    space_proximity_tf[i][j][k] = 1.0f;
                }
            }
        }
        glDeleteTextures(1, &m_tf_texture);
        glGenTextures(1, &m_tf_texture);
        glBindTexture(GL_TEXTURE_3D, m_tf_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 256, 256, 256, 0, GL_RED,  GL_FLOAT, color_proximity_tf);
        glBindTexture(GL_TEXTURE_3D, 0);

        glDeleteTextures(1, &m_space_prox_tf_texture);
        glGenTextures(1, &m_space_prox_tf_texture);
        glBindTexture(GL_TEXTURE_3D, m_space_prox_tf_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 256, 256, 256, 0, GL_RED,  GL_FLOAT, space_proximity_tf);
        glBindTexture(GL_TEXTURE_3D, 0);

        /*
        uint32_t* tf = (uint32_t*)malloc(256);
        int threshold = (int) (tf_rgb_slider_value*256/100)
        for(int i = 0; i < 256; i++)
        {
            if (i < threshold)
                tf[i] = 1
            else
                th[i] = 0;
        }
        */

    }
    else {
        throw std::runtime_error("Unrecognised extension '" + extension + "'.");
    }

}


/*!
 * \brief Create a noise texture with the size of the viewport.
 */
void RayCastVolume::create_noise(void)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int width = viewport[2];
    const int height = viewport[3];

    std::srand(std::time(NULL));
    unsigned char noise[width * height];

    for (unsigned char *p = noise; p <= noise + width * height; ++p) {
        *p = std::rand() % 256;
    }

    glDeleteTextures(1, &m_noise_texture);
    glGenTextures(1, &m_noise_texture);
    glBindTexture(GL_TEXTURE_2D, m_noise_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, noise);
    glBindTexture(GL_TEXTURE_2D, 0);
}


/*!
 * \brief Render the bounding box.
 */
void RayCastVolume::paint(void)
{
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, m_volume_texture);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_noise_texture);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, m_tf_texture);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, m_space_prox_tf_texture);

    m_cube_vao.paint();
}


/*!
 * \brief Range of the image, in intensity value.
 * \return A pair, holding <minimum, maximum>.
 */
std::pair<double, double> RayCastVolume::range() {
    return m_range;
}


/*!
 * \brief Scale factor to model space.
 *
 * Scale the bounding box such that the longest side equals 1.
 */
float RayCastVolume::scale_factor(void)
{
    auto e = m_scaling * m_spacing;
    return std::max({e.x(), e.y(), e.z()});
}

uint32_t RayCastVolume::rgb(int x, int y, int z, int size)
{
    if (x < size*tf_threshold && y < size*tf_threshold && z < size*tf_threshold)
       return ((uint32_t)x << 16 | (uint32_t)y << 8 | (uint32_t)z);
    return (uint32_t)0;
}

void RayCastVolume::update_volume_texture()
{
    m_scaling = volume->size();
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, m_volume_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, m_scaling.x(),m_scaling.y(),m_scaling.z(),0,GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, volume->data());
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void RayCastVolume::update_space_prox_texture()
{
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, m_space_prox_tf_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 256, 256, 256, 0, GL_RED,  GL_FLOAT, space_proximity_tf);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void RayCastVolume::update_color_prox_texture()
{
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, m_tf_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 256, 256, 256, 0, GL_RED,  GL_FLOAT, color_proximity_tf);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void RayCastVolume::set_space_proximity_tf()
{
    int x,y,z;
    if (j==0)
    {
        x=0;y=0;z=0;
    }
    else if(j==1)
    {
        x=100;y=100;z=30;
    }
    else if(j==2)
    {
        x=80;y=200;z=0;

    }
    printf("%d %d %d\n",x,y,z);
    j++;
    
    int min_red = 0;
    int max_red = 256;

    int min_blue = 0;
    int max_blue = 256;

    int min_green = 0;
    int max_green = 256;
    
    for(int i = min_red; i < max_red; i++)
    {
        for(int j = min_green; j < max_green; j++)
        {
            for(int k = min_blue; k < max_blue; k++)
            {
                if (eucl_dist(i,j,k,x,y,z)<=SPACE_PROX_TF_DEFAULT_RADIUS)
                    space_proximity_tf[k][j][i] = 0.0f;

            }
        }
    }
    update_space_prox_texture();
}


void RayCastVolume::set_color_proximity_tf(QRgb rgb)
{
    int red = qRed(rgb);
    int green = qGreen(rgb);
    int blue = qBlue(rgb);
    if (i==0)
    {
        red = 254; green = 254; blue = 254;
    }
    else if(i==1)
    {
        red = 253; green = 210; blue = 233;
    }
    else if(i==2)
    {
        red = 190; green = 128; blue = 203;

    }
    else if(i==3)
    {
        red = 206; green = 165; blue = 214;
    }
    else if(i==4)
    {
        red = 197; green = 137; blue = 207;
    }
    i++;
    printf("%d %d %d\n", red, green, blue);
    
    int min_red = std::max((int)(red-COLOR_PROX_TF_DEFAULT_RADIUS), 0);
    int max_red = std::min((int)(red+COLOR_PROX_TF_DEFAULT_RADIUS), 256);

    int min_blue = std::max((int)(blue-COLOR_PROX_TF_DEFAULT_RADIUS), 0);
    int max_blue = std::min((int)(blue+COLOR_PROX_TF_DEFAULT_RADIUS), 256);

    int min_green = std::max((int)(green-COLOR_PROX_TF_DEFAULT_RADIUS), 0);
    int max_green = std::min((int)(green+COLOR_PROX_TF_DEFAULT_RADIUS), 256);
    
    for(int i = min_red; i < max_red; i++)
    {
        for(int j = min_green; j < max_green; j++)
        {
            for(int k = min_blue; k < max_blue; k++)
            {
                if (eucl_dist(i,j,k,red,green,blue)<=COLOR_PROX_TF_DEFAULT_RADIUS)
                    color_proximity_tf[k][j][i] = 0.0f;

            }
        }
    }
    update_color_prox_texture();


}

float eucl_dist(int a, int b, int c, int x, int y, int z)
{
    return sqrt(pow(a-x, 2)+pow(b-y, 2)+pow(c-z, 2));
}
