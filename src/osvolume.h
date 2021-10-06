#include <string>
#include <vector>
#include <map>

#include <QVector3D>
#include <openslide/openslide.h>

class OSVolume {

    public:
    OSVolume(const std::string& filename);

    QVector3D size();

    QVector3D low_res_size();

    int load_best_res();

    int levels, curr_level;

    uint32_t *low_res_data;

    uint32_t *data;


    private:
    QVector3D _low_res_size;
    openslide_t* image;
    int64_t width, height, depth;  // stores current values

    // map keys: width, height, size, num_voxels
    // NOTE: assumes 4 bytes per voxel.
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
