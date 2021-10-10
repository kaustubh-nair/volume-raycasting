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


    private:
    uint32_t* _data;
    uint32_t* _low_res_data;
    QVector3D _low_res_size;

    // scaling and offset as a fraction of the original full volume;
    // used for determining size of zoomed-in volume
    // z scaling and offset are ignored for now
    QVector3D _scaling_factor;
    QVector3D _scaling_offset;

    uint32_t* zoomed_in(uint32_t* data);

    openslide_t* image;

    // map keys: width, height, size, num_voxels
    // NOTE: assumes 4 bytes per voxel.
    // vector ordered in decreasing order of resolution
    std::vector<std::map<std::string, int64_t>> level_info;

    // for resolution determination; size in bytes
    // use 75% of total vram for a conservative estimate
    // TODO: hardcoded
    uint64_t vram = 1024*1024*1024;         

    void determine_best_level();
    void store_level_info(openslide_t* image, int levels);
    void duplicate_data();
    void load_volume(int l);

};
