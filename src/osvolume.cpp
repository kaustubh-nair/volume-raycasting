#include "osvolume.h"

OSVolume::OSVolume(const std::string& filename)
{
    _depth = 32;

    openslide_t *image = openslide_open(filename.c_str());

    _levels = openslide_get_level_count(image);
    _curr_level = 0;

    openslide_get_level_dimensions(image, _curr_level, &_width, &_height);

    long long int size = _width*_height*4;
    _data = (uint32_t*)malloc(size);
    openslide_read_region(image, _data, 0, 0, _curr_level, _width, _height);
    const char * error = openslide_get_error(image);
    printf("%s \n", error);
//    printf("Debug : %ld \n", _data[0]);

    // duplicate data multiple times
    std::vector<uint32_t> *data3d = new std::vector<uint32_t>(_data, _data + _width*_height);
    std::vector<uint32_t> temp(data3d->begin(), data3d->end());
    for(int i = 0; i < _depth-1; i++)
        std::copy(data3d->begin(),data3d->end(), std::back_inserter(temp));
    std::swap(temp, *data3d);
    // clear memory
    temp = std::vector<uint32_t>();
    free(_data);
    _data = &(*data3d)[0];

    printf("Image loaded! levels: %d width: %ld height %ld\n ", _levels, _width, _height);
}

uint32_t* OSVolume::data()
{
    return _data;
}

QVector3D OSVolume::size()
{
    return QVector3D(_width, _height, _depth);
}

