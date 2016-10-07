#ifndef MEINEKRAFT_TEAPOT_H
#define MEINEKRAFT_TEAPOT_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Teapot: public Entity {
public:
    Teapot();
    ~Teapot();

    virtual void update(uint64_t delta, const std::shared_ptr<Camera> camera) override;

    RenderComponent render_comp;

    constexpr uint64_t generate_entity_id() {
        // TODO: Iterate generate the next entity id
        return 2;
    }
};

#endif //MEINEKRAFT_TEAPOT_H
