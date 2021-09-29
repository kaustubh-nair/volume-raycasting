#include "osvolume.h"

OSVolume::OSVolume(const std::string& filename)
{
    openslide_t *image = openslide_open(filename.c_str());

    _levels = openslide_get_level_count(image);
    _curr_level = _levels-1;

    openslide_get_level_dimensions(image, _curr_level, &_width, &_height);
    printf("levels %ld \n", _levels);
}

std::vector<unsigned char> OSVolume::data()
{
    return _data;
}

QVector3D OSVolume::size()
{
    _depth = 1;
    return QVector3D(_width, _height, _depth);
}

