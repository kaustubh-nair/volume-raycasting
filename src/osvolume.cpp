#include "osvolume.h"

OSVolume::OSVolume(const std::string& filename)
{
    image = openslide_open(filename.c_str());

    levels = openslide_get_level_count(image);

    store_level_info(image, levels);

    // lowest resolution is loaded fully initially.
    // Be careful while changing this - low_res_data values and width/depth/height are initialized based on this.
    _curr_level = levels-1;
    _scaling_factor = QVector3D(1.0, 1.0, 1.0);
    _scaling_offset = QVector3D(0.0, 0.0, 0.0);
    load_volume(_curr_level);

    _low_res_data = _data;
    _low_res_size = QVector3D(level_info[_curr_level]["width"], level_info[_curr_level]["height"], level_info[_curr_level]["depth"]);

}

QVector3D OSVolume::size()
{
    return QVector3D(
            _scaling_factor.x()*level_info[_curr_level]["width"],
            _scaling_factor.y()*level_info[_curr_level]["height"],
            _scaling_factor.z()*level_info[_curr_level]["depth"]
    );
}

void OSVolume::load_volume(int l)
{
    _data = nullptr;

    _curr_level = l;

    int64_t width = level_info[_curr_level]["width"]*_scaling_factor.x();
    int64_t height = level_info[_curr_level]["height"]*_scaling_factor.y();

    int w_offset = level_info[_curr_level]["width"]*_scaling_offset.x();
    int h_offset = level_info[_curr_level]["height"]*_scaling_offset.y();

	long long int size = width*height*sizeof(uint32_t);
    _data = (uint32_t*)malloc(size);


    openslide_read_region(image, _data, w_offset, h_offset, _curr_level, width, height);

    duplicate_data(&_data);

    printf("\nImage loaded! Levels: %d Width: %ld Height: %ld Depth: %ld Current Level: %d\n\n", levels,
            level_info[_curr_level]["width"],
            level_info[_curr_level]["height"],
            level_info[_curr_level]["depth"],
            _curr_level
    );
}

// duplicate data "depth" times
void OSVolume::duplicate_data(uint32_t** d)
{
    int64_t width = level_info[_curr_level]["width"]*_scaling_factor.x();
    int64_t height = level_info[_curr_level]["height"]*_scaling_factor.y();
    int64_t depth = level_info[_curr_level]["depth"]*_scaling_factor.z();

    std::vector<uint32_t> *data3d = new std::vector<uint32_t>(*d, *d + width*height);
    std::vector<uint32_t> temp(data3d->begin(), data3d->end());
    for(int i = 0; i < depth-1; i++)
        std::copy(data3d->begin(),data3d->end(), std::back_inserter(temp));
    std::swap(temp, *data3d);
    // clear memory
    temp = std::vector<uint32_t>();
    free(*d);
    *d = &(*data3d)[0];
}

void OSVolume::store_level_info(openslide_t* image, int levels)
{
    int64_t w, h;
    printf("\n\nLEVEL INFO: \n");
    for(int i = 0; i < levels; i++)
    {
        std::map<std::string, int64_t> m;
        openslide_get_level_dimensions(image, i, &w, &h);
        m["width"] = w;
        m["height"] = h;
        m["depth"] = 32;        //TODO hardcoded - loads and duplicates single volume
        m["num_voxels"] = w*h;
        m["size"] = m["num_voxels"]*4/1024; // KB
        printf("Level: %d Width: %d Height: %d Depth: %d\n", i, m["width"], m["height"], m["depth"]);
        level_info.push_back(m);
    }
    printf("\n\n");
}

int OSVolume::load_best_res()
{
    int64_t depth = level_info[_curr_level]["depth"];
    //int64_t curr_size = width*height;
   
    // assumes same size per slide, but should be ok?
    // reconsider when switching to 3D
    // use 75% of total vram for a conservative estimate
    int64_t available_size = (int64_t)(vram*0.75)/depth;



    // iterate from highest resolution, and load it if it fits.
    for(int i = 0; i < (int)level_info.size(); i++)
    {
        printf("attempting to load %d %d %d", i, level_info[i]["size"], available_size);
        if (level_info[i]["size"] < available_size)
        {
            if (_curr_level == i) break;

            if (_data != _low_res_data)
                free(_data);
            load_volume(i);
            break;
        }
    }
    return _curr_level;
}

uint32_t *OSVolume::zoomed_in(uint32_t *data)
{
    // no zooming required
    if (_scaling_factor.x() == 1.0 && _scaling_factor.y() == 1.0 && _scaling_factor.z() == 1.0)
        return data;

    int64_t width = level_info[_curr_level]["width"];
    int64_t height = level_info[_curr_level]["height"];
    int64_t depth = level_info[_curr_level]["depth"];

    int w_small = level_info[_curr_level]["width"]*_scaling_factor.x();
    int h_small = level_info[_curr_level]["height"]*_scaling_factor.y();
    int d_small = level_info[_curr_level]["depth"]*_scaling_factor.z();

    int w_offset = level_info[_curr_level]["width"]*_scaling_offset.x();
    int h_offset = level_info[_curr_level]["height"]*_scaling_offset.y();
    int d_offset = level_info[_curr_level]["depth"]*_scaling_offset.z();

    uint32_t* zoomed_in = (uint32_t*)malloc(w_small*h_small*d_small*sizeof(uint32_t));

    int ptr = 0;
    for(int i = d_offset; i < d_small; i++)
    {
        for(int j = h_offset; j < h_small; j++)
        {
            std::copy(_low_res_data+w_offset + (j*width) + (i*width*height), _low_res_data+w_offset + w_small + (j*width) + (i*width*height), zoomed_in+ptr);
            ptr += w_small;
        }
    }

    return zoomed_in;
}

void OSVolume::zoom_in()
{
    if (_scaling_factor.x() <= 0.01 || _scaling_factor.y() <= 0.01 || _scaling_factor.z() <= 0.01)
        return;

    // Do only x-y zoom for now
    _scaling_factor -= QVector3D(_scaling_factor_value,_scaling_factor_value,_scaling_factor_value);
    _scaling_offset += QVector3D(_scaling_offset_value, _scaling_offset_value, _scaling_offset_value);

}

void OSVolume::zoom_out()
{
    // Do only x-y zoom for now
    _scaling_factor += QVector3D(_scaling_factor_value, _scaling_factor_value, _scaling_factor_value);
    _scaling_offset -= QVector3D(_scaling_offset_value, _scaling_offset_value, _scaling_offset_value);

    // cap to 1.0
    if (_scaling_factor.x() > 1.0) _scaling_factor.setX(1.0);
    if (_scaling_factor.y() > 1.0) _scaling_factor.setY(1.0);
    if (_scaling_factor.z() > 1.0) _scaling_factor.setZ(1.0);

    // min 0
    if (_scaling_offset.x() < 0.0) _scaling_offset.setX(0.0);
    if (_scaling_offset.y() < 0.0) _scaling_offset.setY(0.0);
    if (_scaling_offset.z() < 0.0) _scaling_offset.setZ(0.0);


}

void OSVolume::switch_to_low_res()
{
    _curr_level = levels-1;
}

uint32_t* OSVolume::data()
{
    if (_curr_level == levels-1)
        return zoomed_in();
    return _data;
}


void OSVolume::move_up()
{
    _scaling_offset.setY(_scaling_offset.y()+4*_scaling_offset_value);
    if (_scaling_offset.y() > 1.0) _scaling_offset.setY(1.0);

}
void OSVolume::move_down()
{
    _scaling_offset.setY(_scaling_offset.y()-4*_scaling_offset_value);
    if (_scaling_offset.y() < 0.0) _scaling_offset.setY(0.0);

}
void OSVolume::move_right()
{
    _scaling_offset.setX(_scaling_offset.x()+4*_scaling_offset_value);
    if (_scaling_offset.x() > 1.0) _scaling_offset.setX(1.0);

}
void OSVolume::move_left()
{
    _scaling_offset.setX(_scaling_offset.x()-_scaling_offset_value);
    if (_scaling_offset.x() < 0.0) _scaling_offset.setX(0.0);

}

uint32_t* OSVolume::zoomed_in()
{
    int w_small = level_info[_curr_level]["width"]*_scaling_factor.x();
    int h_small = level_info[_curr_level]["height"]*_scaling_factor.y();
    int d_small = level_info[_curr_level]["depth"]*_scaling_factor.z();

    int w_offset = level_info[_curr_level]["width"]*_scaling_offset.x();
    int h_offset = level_info[_curr_level]["height"]*_scaling_offset.y();
    int d_offset = level_info[_curr_level]["depth"]*_scaling_offset.z();

	long long int size = w_small*h_small*sizeof(uint32_t);
    uint32_t* d = (uint32_t*)malloc(size);

    openslide_read_region(image, d, w_offset, h_offset, _curr_level, w_small, h_small);
    duplicate_data(&d);
    return d;
}
