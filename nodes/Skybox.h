#ifndef MEINEKRAFT_SKYBOX_H
#define MEINEKRAFT_SKYBOX_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Skybox: public Entity {
public:
    Skybox();
    ~Skybox();

    RenderComponent render_comp;
};

#endif //MEINEKRAFT_SKYBOX_H
