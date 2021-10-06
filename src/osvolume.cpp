#include "osvolume.h"

OSVolume::OSVolume(const std::string& filename)
{
    depth = 32;

    image = openslide_open(filename.c_str());

    levels = openslide_get_level_count(image);

    store_level_info(image, levels);

    // lowest resolution is loaded fully initially.
    // Be careful while removing this - low_res_data values and width/depth/height depend on this.
    load_volume(levels-1);

    low_res_data = data;
    _low_res_size = QVector3D(width, height, depth);

    printf("Image loaded! levels: %d width: %ld height %ld\n ", levels, width, height);
}

QVector3D OSVolume::size()
{
    return QVector3D(width, height, depth);
}

QVector3D OSVolume::low_res_size()
{
    return _low_res_size;
}

void OSVolume::load_volume(int l)
{
    data = nullptr;

    curr_level = l;

    width = level_info[curr_level]["width"];
    height = level_info[curr_level]["height"];

	long long int size = width*height*4;
    data = (uint32_t*)malloc(size);

    openslide_read_region(image, data, 0, 0, curr_level, width, height);

    duplicate_data();
}

// duplicate data "depth" times
void OSVolume::duplicate_data()
{
    std::vector<uint32_t> *data3d = new std::vector<uint32_t>(data, data + width*height);
    std::vector<uint32_t> temp(data3d->begin(), data3d->end());
    for(int i = 0; i < depth-1; i++)
        std::copy(data3d->begin(),data3d->end(), std::back_inserter(temp));
    std::swap(temp, *data3d);
    // clear memory
    temp = std::vector<uint32_t>();
    free(data);
    data = &(*data3d)[0];
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

int OSVolume::load_best_res()
{
    //int64_t curr_size = width*height;
   
    // assumes same size per slide, but should be ok?
    // use 75% of total vram for a conservative estimate
    int64_t available_size = (int64_t)(vram*0.75)/depth;


    for(int i = 0; i < (int)level_info.size(); i++)
    {
        if (level_info[i]["size"] < available_size)
        {
            if (curr_level == i) break;

            if (data != low_res_data)
                free(data);
            load_volume(i);
            break;
        }
    }
    return curr_level;
}
