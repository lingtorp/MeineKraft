#ifndef MEINEKRAFT_TEXTURE_H
#define MEINEKRAFT_TEXTURE_H

#include <string>

class Texture {
public:
    Texture();

    bool load(std::string filepath, std::string directory);

    uint64_t gl_texture;
    uint64_t gl_texture_type;
    uint64_t gl_texture_location;
    bool loaded_succesfully;
};

#endif //MEINEKRAFT_TEXTURE_H
