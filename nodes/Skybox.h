#ifndef MEINEKRAFT_SKYBOX_H
#define MEINEKRAFT_SKYBOX_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Skybox: public Entity {
public:
    Skybox();
    ~Skybox();

    virtual void update(uint64_t delta, const std::shared_ptr<Camera> camera) override;

    RenderComponent render_comp;

    constexpr uint64_t generate_entity_id() {
        // TODO: Iterate generate the next entity id
        return 0;
    }
};

#endif //MEINEKRAFT_SKYBOX_H
