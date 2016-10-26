#ifndef MEINEKRAFT_TEAPOT_H
#define MEINEKRAFT_TEAPOT_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Teapot: public Entity {
public:
    Teapot();

    RenderComponent render_comp;

    constexpr uint64_t generate_entity_id() {
        // TODO: Iterate generate the next entity id
        return 2;
    }
};

#endif //MEINEKRAFT_TEAPOT_H
