#ifndef MEINEKRAFT_TEAPOT_H
#define MEINEKRAFT_TEAPOT_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Teapot: public Entity {
public:
    Teapot(std::string mesh_file, std::string directory);
};

#endif //MEINEKRAFT_TEAPOT_H
