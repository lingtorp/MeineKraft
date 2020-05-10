#include "entity.hpp"

#include <cassert>

#include "../rendering/rendercomponent.hpp"
#include "../rendering/renderer.hpp"
#include "../nodes/physics_system.hpp"
#include "transform.hpp"

static const uint32_t RENDER_SYSTEM_COMPONENT_TAG    = 0b1;
static const uint32_t TRANSFORM_SYSTEM_COMPONENT_TAG = 0b10;
static const uint32_t PHYSICS_SYSTEM_COMPONENT_TAG   = 0b100;
static const uint32_t ACTION_SYSTEM_COMPONENT_TAG    = 0b1000;

Entity::Entity(): id(EntitySystem::instance().new_entity()), components(0) {}

Entity::~Entity() {
    EntitySystem::instance().destroy_entity(id);
    // TODO: Add all of them!
    // if (components & RENDER_SYSTEM_COMPONENT_TAG)    { Renderer::instance().remove_component(id); }
    // if (components & TRANSFORM_SYSTEM_COMPONENT_TAG) { TransformSystem::instance().remove_component(id); }
}

ID Entity::clone() const {
    if (components & RENDER_SYSTEM_COMPONENT_TAG) {
        // TODO: Implement clone function
    }
    return 0;
}

/** Component handling for convenience **/
void Entity::attach_component(const RenderComponent& component) {
    assert(!(components & RENDER_SYSTEM_COMPONENT_TAG));
    components |= RENDER_SYSTEM_COMPONENT_TAG;
    MeineKraft::instance().renderer->add_component(component, id);
}

void Entity::attach_component(const TransformComponent& component) {
    assert(!(components & TRANSFORM_SYSTEM_COMPONENT_TAG));
    components |= TRANSFORM_SYSTEM_COMPONENT_TAG;
    TransformSystem::instance().add_component(component, id);
}

void Entity::attach_component(const PhysicsComponent& component) {
    PhysicsSystem::instance().add_component(component, id);
}

void Entity::attach_component(const ActionComponent& component) {
    ActionSystem::instance().add_component(component);
}

// Detachs
void Entity::detach_component(const RenderComponent& component) {
    MeineKraft::instance().renderer->remove_component(id);
}

void Entity::detach_component(const TransformComponent& component) {
    TransformSystem::instance().remove_component(id);
}
