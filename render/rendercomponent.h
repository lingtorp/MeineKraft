#ifndef MEINEKRAFT_RENDERCOMPONENT_H
#define MEINEKRAFT_RENDERCOMPONENT_H

#include "primitives.h"

class Renderer;
class Entity;

class Component {
public:
    /// Called when the component is added to the Entity
    virtual void did_attach_to_entity(Entity *entity) {}
    /// Called when the component is removed to the Entity
    virtual void did_deattach_from_entity(Entity *entity) {}
    /// Called once every frame
    virtual void update() {}
};

/**
 * RenderComponent is added to a Entity that has a visual presence
 * It provides a interface for the Renderer to use when presenting
 * Entities that has a visual presence.
 *
 * Also provides the Entity a nice interface to manipulate its visual data
 */
class RenderComponent: public Component {
public:
    GraphicsState graphics_state;
    Entity *entity;

    /// Creates a RenderComponent with the mesh of a .obj file
    RenderComponent(Entity *entity);

    void update() override;

    void set_obj_mesh(std::string mesh_file, std::string directory_file);

    void did_attach_to_entity(Entity *entity) override {

    }
};

#endif //MEINEKRAFT_RENDERCOMPONENT_H
