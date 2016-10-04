#ifndef MEINEKRAFT_BLOCK_H
#define MEINEKRAFT_BLOCK_H

#include "entity.h"
#include "../render/rendercomponent.h"

class Block: public Entity {
public:
    Block();
    ~Block();

    virtual void update(uint64_t delta, const std::shared_ptr<Camera> camera) override;

    RenderComponent render_comp;

    constexpr uint64_t generate_entity_id() {
        // TODO: Iterate generate the next entity id
        return 1;
    }
};

#endif //MEINEKRAFT_BLOCK_H
