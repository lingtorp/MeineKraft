#ifndef MEINEKRAFT_TEAPOT_H
#define MEINEKRAFT_TEAPOT_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Model: public Entity {
public:
    Model(const std::string& directory, const std::string& file);
};

#endif //MEINEKRAFT_TEAPOT_H
