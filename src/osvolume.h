#include <string>
#include <vector>

#include <QVector3D>
#include <openslide/openslide.h>

class OSVolume {

    public:
    OSVolume(const std::string& filename);

    std::vector<unsigned char> data();

    QVector3D size();

    private:
    std::vector<unsigned char> _data;
    int64_t _width, _height, _depth;
    int _levels, _curr_level;

};
