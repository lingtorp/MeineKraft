#include <iostream>
#include "Skybox.h"
#include "../render/camera.h"

Skybox::Skybox(): Entity(), render_comp{this} {
//    render_comp.set_cube_map_texture(Texture::SKYBOX);
    scale = 300;
}

Skybox::~Skybox() {
    // TODO: Dealloc RenderComp
}

void Skybox::update(uint64_t delta, const std::shared_ptr<Camera> camera) {
    /// Update all components
    position = camera->position;
    // render_comp.update(delta)
}
