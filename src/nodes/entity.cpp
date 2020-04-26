#include "entity.hpp"

#include "../rendering/rendercomponent.hpp"
#include "../rendering/renderer.hpp"
#include "../nodes/physics_system.hpp"
#include "transform.hpp"

Entity::Entity(): id(EntitySystem::instance().new_entity()) {}

Entity::~Entity() {
    EntitySystem::instance().destroy_entity(id);
    // TODO: Add all of them?
    // if (components & RENDER_COMPONENT) { Renderer::instance().remove_component(id); }
}

/** Component handling for convenience **/
void Entity::attach_component(const RenderComponent& component) const {
    MeineKraft::instance().renderer->add_component(component, id);
}

void Entity::attach_component(const TransformComponent& component) const {
    TransformSystem::instance().add_component(component, id);
}

void Entity::attach_component(const PhysicsComponent& component) const {
    PhysicsSystem::instance().add_component(component, id);
}

void Entity::attach_component(const ActionComponent& component) const {
    ActionSystem::instance().add_component(component);
}

void Entity::detach_component(const RenderComponent& component) const {
    MeineKraft::instance().renderer->remove_component(id);
}

void Entity::detach_component(const TransformComponent& component) const {
    TransformSystem::instance().remove_component(id);
}
