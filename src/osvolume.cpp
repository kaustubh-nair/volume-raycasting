#include "osvolume.h"

OSVolume::OSVolume(const std::string& filename)
{
    image = openslide_open(filename.c_str());

    levels = openslide_get_level_count(image);

    store_level_info(image, levels);

    // lowest resolution is loaded fully initially.
    // Be careful while changing this - low_res_data values and width/depth/height are initialized based on this.
    curr_level = levels-1;
    load_volume(curr_level);

    _low_res_data = _data;
    _low_res_size = QVector3D(level_info[curr_level]["width"], level_info[curr_level]["height"], level_info[curr_level]["depth"]);
    _low_res_scaling = QVector3D(1.0, 1.0, 1.0);
    _low_res_offset = QVector3D(0.0, 0.0, 0.0);

    printf("Image loaded! levels: %d width: %ld height %ld depth %ld\n ", levels,
            level_info[curr_level]["width"],
            level_info[curr_level]["height"],
            level_info[curr_level]["depth"]
    );
}

QVector3D OSVolume::size()
{
    return QVector3D(level_info[curr_level]["width"], level_info[curr_level]["height"], level_info[curr_level]["depth"]);
}

void OSVolume::load_volume(int l)
{
    free(_data);
    _data = nullptr;

    curr_level = l;

    int64_t width = level_info[curr_level]["width"];
    int64_t height = level_info[curr_level]["height"];

	long long int size = width*height*4;
    _data = (uint32_t*)malloc(size);

    openslide_read_region(image, _data, 0, 0, curr_level, width, height);

    duplicate_data();
}

// duplicate data "depth" times
void OSVolume::duplicate_data()
{
    int64_t height = level_info[curr_level]["height"];
    int64_t width = level_info[curr_level]["width"];
    int64_t depth = level_info[curr_level]["depth"];

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
        m["depth"] = 32;        //TODO hardcoded - loads and duplicates single volume
        m["num_voxels"] = w*h;
        m["size"] = m["num_voxels"]*4;
        level_info.push_back(m);
    }
}

int OSVolume::load_best_res()
{
    int64_t height = level_info[curr_level]["height"];
    int64_t width = level_info[curr_level]["width"];
    int64_t depth = level_info[curr_level]["depth"];
    //int64_t curr_size = width*height;
   
    // assumes same size per slide, but should be ok?
    // use 75% of total vram for a conservative estimate
    int64_t available_size = (int64_t)(vram*0.75)/depth;


    // iterate from highest resolution, and load it if it fits.
    for(int i = 0; i < (int)level_info.size(); i++)
    {
        if (level_info[i]["size"] < available_size)
        {
            if (curr_level == i) break;

            if (_data != _low_res_data)
                free(_data);
            load_volume(i);
            break;
        }
    }
    return curr_level;
}

uint32_t *OSVolume::zoomed_in_data(uint32_t *data)
{
    return _low_res_data;
    int64_t height = level_info[curr_level]["height"];
    int64_t width = level_info[curr_level]["width"];
    int64_t depth = level_info[curr_level]["depth"];

    int w = level_info[level_info.size()-1]["width"]*_low_res_scaling.x();
    int h = level_info[level_info.size()-1]["height"]*_low_res_scaling.y();
    int d = level_info[level_info.size()-1]["depth"]*_low_res_scaling.z();

    int w_offset = level_info[level_info.size()-1]["width"]*_low_res_offset.x();
    int h_offset = level_info[level_info.size()-1]["height"]*_low_res_offset.y();
    int d_offset = level_info[level_info.size()-1]["depth"]*_low_res_offset.z();

    // remove x4 ?
    // TODO: memory leak here! DANGER!
    uint32_t* zoomed_in_data = (uint32_t*)malloc(width*depth*height*4);
    std::copy(_low_res_data, _low_res_data+(width*height*depth*4), zoomed_in_data);
    return zoomed_in_data;

    int ptr = 0;
    for(int k = d_offset; k < d; k++)
    {
        for(int j = h_offset; j < h; j++)
        {
            std::copy(_low_res_data+w_offset+(j*h_offset)+(k*d_offset), _low_res_data+w_offset+w+(j*h_offset)+(k*d_offset), zoomed_in_data+ptr);
            ptr += w;
        }
    }


    return zoomed_in_data;
}

void OSVolume::zoom_in()
{
    if (_low_res_scaling.x() <= 0.1 || _low_res_scaling.y() <= 0.1 || _low_res_scaling.z() <= 0.1)
        return;

    // Do only x-y zoom for now
    _low_res_scaling -= QVector3D(0.1, 0.1, 0.0);
    _low_res_offset += QVector3D(0.05, 0.05, 0.0);

}

void OSVolume::zoom_out()
{
    // Do only x-y zoom for now
    _low_res_scaling += QVector3D(0.1, 0.1, 0.0);
    _low_res_offset -= QVector3D(0.05, 0.05, 0.0);

    // cap to 1.0
    if (_low_res_scaling.x() > 1.0) _low_res_scaling.setX(1.0);
    if (_low_res_scaling.y() > 1.0) _low_res_scaling.setY(1.0);
    if (_low_res_scaling.z() > 1.0) _low_res_scaling.setZ(1.0);

}

void OSVolume::switch_to_low_res()
{
    curr_level = levels-1;
}

uint32_t* OSVolume::data()
{
    if (curr_level == levels-1)
        return _low_res_data;
    return _data;
}
