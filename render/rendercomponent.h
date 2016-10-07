#ifndef MEINEKRAFT_RENDERCOMPONENT_H
#define MEINEKRAFT_RENDERCOMPONENT_H

#include "primitives.h"

class Renderer;
class Entity;

class RenderComponent {
public:
    uint64_t id;
    GraphicsState graphics_state;
    Entity *entity;

    /// Creates a RenderComponent with the mesh of a obj file
    RenderComponent(Entity *entity, std::string mesh_file);

    /// Creates a RenderComponent with the mesh of a Cube
    RenderComponent(Entity *entity);

    // RenderComponent - with sprite file/images/
    void remove_component();

    void set_cube_map_texture(Texture texture);
};

#endif //MEINEKRAFT_RENDERCOMPONENT_H
