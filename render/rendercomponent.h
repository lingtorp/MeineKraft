#ifndef MEINEKRAFT_RENDERCOMPONENT_H
#define MEINEKRAFT_RENDERCOMPONENT_H

#include "primitives.h"

class Renderer;
class Entity;

class RenderComponent {
public:
    GraphicsState graphics_state;
    Entity *entity;

    // Defaults to the mesh of a cube
    RenderComponent(Entity *entity);
    // RenderComponent - with obj file
    // RenderComponent - with sprite file
    void remove_component();
};

#endif //MEINEKRAFT_RENDERCOMPONENT_H
