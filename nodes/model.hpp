#pragma once
#ifndef MEINEKRAFT_TEAPOT_H
#define MEINEKRAFT_TEAPOT_H

#include "entity.hpp"
#include "../render/rendercomponent.hpp"

class Model: public Entity {
public:
    Model(const std::string& directory, const std::string& file);
};

#endif //MEINEKRAFT_TEAPOT_H
