#include <string>
#include <vector>
#include <map>

#include <QVector3D>
#include <openslide/openslide.h>

class OSVolume {

    public:
    OSVolume(const std::string& filename);

    QVector3D size();

    int load_best_res();

    int levels, _curr_level;

    uint32_t *data();

    void zoom_in();

    void zoom_out();

    void switch_to_low_res();

    void move_right();
    
    void move_up();

    void move_down();

    void move_left();

    void set_vram(int value)
    {
         vram = value*1024;
    }


    private:
    uint32_t* _data;
    uint32_t* _low_res_data;
    QVector3D _low_res_size;

    // scaling and offset as a fraction of the original full volume;
    // used for determining size of zoomed-in volume
    // z scaling and offset are ignored for now
    QVector3D _scaling_factor;
    QVector3D _scaling_offset;
    double _scaling_factor_value = 0.06;
    double _scaling_offset_value = 0.03;

    uint32_t* zoomed_in(uint32_t* data);

    openslide_t* image;

    // map keys: width, height, size, num_voxels
    // NOTE: assumes 4 bytes per voxel.
    // vector ordered in decreasing order of resolution
    std::vector<std::map<std::string, int64_t>> level_info;

    // for resolution determination; size in KB
    // TODO: WARNING: change default value here if changing in UI (passing it in Mainwindow() causes wierd segfault)
    uint64_t vram = 1024*1024;

    void determine_best_level();
    void store_level_info(openslide_t* image, int levels);
    void load_volume(int l);
    void duplicate_data(uint32_t** d);
    uint32_t* zoomed_in();


};
