#ifndef MEINEKRAFT_TEAPOT_H
#define MEINEKRAFT_TEAPOT_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Model: public Entity {
public:
    Model(std::string mesh_file, std::string directory);
};

#endif //MEINEKRAFT_TEAPOT_H
