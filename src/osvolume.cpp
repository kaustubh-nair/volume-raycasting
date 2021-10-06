#include "osvolume.h"

OSVolume::OSVolume(const std::string& filename)
{
    depth = 32;

    openslide_t *image = openslide_open(filename.c_str());

    levels = openslide_get_level_count(image);

    store_level_info(image, levels);

    curr_level = 0;
    width = level_info[curr_level]["width"];
    height = level_info[curr_level]["height"];

	long long int size = width*height*4;
    _data = (uint32_t*)malloc(size);
    openslide_read_region(image, _data, 0, 0, curr_level, width, height);

    duplicate_data();
    printf("Image loaded! levels: %d width: %ld height %ld\n ", levels, width, height);
}

uint32_t* OSVolume::data()
{
    return _data;
}

QVector3D OSVolume::size()
{
    return QVector3D(width, height, depth);
}

// duplicate data "depth" times
void OSVolume::duplicate_data()
{
    std::vector<uint32_t> *data3d = new std::vector<uint32_t>(_data, _data + width*height);
    std::vector<uint32_t> temp(data3d->begin(), data3d->end());
    for(int i = 0; i < depth-1; i++)
        std::copy(data3d->begin(),data3d->end(), std::back_inserter(temp));
    std::swap(temp, *data3d);
    // clear memory
    temp = std::vector<uint32_t>();
    free(_data);
    _data = &(*data3d)[0];
}

void OSVolume::store_level_info(openslide_t* image, int levels)
{
    int64_t w, h;
    for(int i = 0; i < levels; i++)
    {
        std::map<std::string, int64_t> m;
        openslide_get_level_dimensions(image, i, &w, &h);
        m["width"] = w;
        m["height"] = h;
        m["num_voxels"] = w*h;
        m["size"] = m["num_voxels"]*4;
        level_info.push_back(m);
    }
}
