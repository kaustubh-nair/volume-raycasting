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


float eucl_dist(int a, int b, int c, int x, int y, int z)
{
    return sqrt(pow(a-x, 2)+pow(b-y, 2)+pow(c-z, 2));
}

/*!
 * \brief Create a two-unit cube mesh as the bounding box for the volume.
 */
RayCastVolume::RayCastVolume(void)
    : m_volume_texture {0}
    , m_noise_texture {0}
    , m_tf_texture {0}
    , m_segment_opacity_texture {0}
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

        initialize_texture_data();

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


        glDeleteTextures(1, &m_tf_texture);
        glGenTextures(1, &m_tf_texture);
        glBindTexture(GL_TEXTURE_3D, m_tf_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // TODO: recheck if interpolation is needed
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, COLOR_TF_DIMENSION, COLOR_TF_DIMENSION, COLOR_TF_DIMENSION, 0, GL_RED,  GL_FLOAT, color_proximity_tf);
        glBindTexture(GL_TEXTURE_3D, 0);

        glDeleteTextures(1, &m_location_tf_texture);
        glGenTextures(1, &m_location_tf_texture);
        glBindTexture(GL_TEXTURE_3D, m_location_tf_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // TODO: recheck if interpolation is needed
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, LOCATION_TF_DIMENSION, LOCATION_TF_DIMENSION, LOCATION_TF_DIMENSION, 0, GL_RED,  GL_FLOAT, location_tf);
        glBindTexture(GL_TEXTURE_3D, 0);

        for(int i = 0; i < 3; i++)
        {
            segment_opacity_tf[i] = 1.0f;
        }
        glDeleteTextures(1, &m_segment_opacity_texture);
        glGenTextures(1, &m_segment_opacity_texture);
        glBindTexture(GL_TEXTURE_1D, m_segment_opacity_texture);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, MAX_NUM_SEGMENTS, 0, GL_RED, GL_FLOAT, segment_opacity_tf);
        glBindTexture(GL_TEXTURE_1D, 0);

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
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, m_location_tf_texture);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_1D, m_segment_opacity_texture);

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
    // this causes a blank screen somehow weird!;
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_volume_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, m_scaling.x(),m_scaling.y(),m_scaling.z(),0,GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, volume->data());
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void RayCastVolume::update_location_tf_texture()
{
    // this causes a blank screen somehow weird!;
    //glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, m_location_tf_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, LOCATION_TF_DIMENSION, LOCATION_TF_DIMENSION, LOCATION_TF_DIMENSION, 0, GL_RED,  GL_FLOAT, location_tf);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void RayCastVolume::update_segment_opacity_texture()
{
    // this causes a blank screen somehow weird!;
    // glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_1D, m_segment_opacity_texture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, MAX_NUM_SEGMENTS, 0, GL_RED, GL_FLOAT, segment_opacity_tf);
    glBindTexture(GL_TEXTURE_1D, 0);
}

void RayCastVolume::update_color_prox_texture()
{
    // this causes a blank screen somehow weird!;
    //glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, m_tf_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, COLOR_TF_DIMENSION, COLOR_TF_DIMENSION, COLOR_TF_DIMENSION, 0, GL_RED,  GL_FLOAT, color_proximity_tf);
    glBindTexture(GL_TEXTURE_3D, 0);
}

/*
void RayCastVolume::set_location_tf()
{
    int x,y,z;
    if (j==0)
    {
        x=0;y=0;z=256;
    }
    else if(j==1)
    {
        x=100;y=100;z=256;
    }
    else if(j==2)
    {
        x=80;y=200;z=256;

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
                    location_tf[k][j][i] = 0.0f;

            }
        }
    }
    update_location_tf_texture();
}
*/


void RayCastVolume::update_color_proximity_tf_data()
{
    
    int red,green,blue;

    
    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            for(int k = 0; k < 256; k++)
            {
                // re-initialize the whole array to deal with deletes
                color_proximity_tf[i][j][k] = 1.0f;

                for(int i = 0; i < color_tf_data.size(); i++)
                {
                    red = qRed(color_tf_data[i].rgb);
                    green = qGreen(color_tf_data[i].rgb);
                    blue = qBlue(color_tf_data[i].rgb);

                    if (eucl_dist(i,j,k,red,green,blue)<=color_tf_data[i].proximity_radius)
                        color_proximity_tf[k][j][i] *= color_tf_data[i].opacity;

                }

            }
        }
    }
    update_color_prox_texture();

}

void RayCastVolume::set_color_proximity_tf_data(QRgb rgb, int id)
{
    ColorTF new_tf = { id, rgb, COLOR_PROX_TF_DEFAULT_RADIUS, 0.5};
    color_tf_data.push_back(new_tf);
    update_color_proximity_tf_data();
}

void RayCastVolume::update_volume_opacity(int opacity)
{
   volume_opacity = opacity/100.0; 
   update_location_tf_data();
}

void RayCastVolume::update_segment_opacity(int id, int opacity)
{
    segment_opacity_tf[id] = opacity/100.0f;
    update_segment_opacity_texture();
}

void RayCastVolume::update_location_tf_data()
{
    for(int i = 0; i < LOCATION_TF_DIMENSION; i++)
    {
        for(int j = 0; j < LOCATION_TF_DIMENSION; j++)
        {
            for(int k = 0; k < LOCATION_TF_DIMENSION; k++)
            {
                location_tf[i][j][k] = volume_opacity;
            }
        }
    }

}

void RayCastVolume::initialize_color_proximity_tf()
{
    for(int i = 0; i < COLOR_TF_DIMENSION; i++)
    {
        for(int j = 0; j < COLOR_TF_DIMENSION; j++)
        {
            for(int k = 0; k < COLOR_TF_DIMENSION; k++)
            {
                color_proximity_tf[i][j][k] = 1.0f;
            }
        }
    }
}

void RayCastVolume::initialize_texture_data()
{
    update_location_tf_data();

    initialize_color_proximity_tf();

    for(int i = 0; i < MAX_NUM_SEGMENTS; i++)
    {
        segment_opacity_tf[i] = 1.0f;
    }


}

void RayCastVolume::update_location_tf(std::vector<Polygon> polygons)
{
    if (polygons.size()==0) 
    {
        update_location_tf_texture(); return;
    }

    for(int i = 0; i < LOCATION_TF_DIMENSION; i++)
    {
        for(int j =0; j < LOCATION_TF_DIMENSION; j++)
        {
            for(int k = 0; k < polygons.size(); k++)
            {
                if (polygons[k].point_is_inside(i/(float)LOCATION_TF_DIMENSION, j/(float)LOCATION_TF_DIMENSION))
                {
                    // replace opacity of full volume, else compose
                    if (location_tf[0][j][i] == volume_opacity)
                        location_tf[0][j][i] = polygons[k].get_opacity();
                    else
                        location_tf[0][j][i] = location_tf[0][j][i]*polygons[k].get_opacity();
                }

            }
        }
    }
    // duplicate z layer for now since TF can only be oriented parallel to volume
    for(int i = 0; i < LOCATION_TF_DIMENSION; i++)
    {
        for(int j =0; j < LOCATION_TF_DIMENSION; j++)
        {
            for(int k = 1; k < LOCATION_TF_DIMENSION; k++)
            {
                location_tf[k][j][i] = location_tf[0][j][i];
            }
        }
    }
    update_location_tf_texture();
}
