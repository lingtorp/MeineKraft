#include "rendercomponent.h"
#include "render.h"

RenderComponent::RenderComponent(Entity *entity): entity(entity), graphics_state{} {
    Render::instance().add_to_batch(*this);
};

void RenderComponent::remove_component() {
    Render::instance().remove_from_batch(*this);
}
