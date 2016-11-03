#include <iostream>
#include "Skybox.h"
#include "../render/camera.h"

Skybox::Skybox(): Entity(generate_entity_id()), render_comp{this} {
//    render_comp.set_cube_map_texture(Texture::SKYBOX);
    scale = 300;
}

Skybox::~Skybox() {
    render_comp.remove_component();
}

void Skybox::update(uint64_t delta, const std::shared_ptr<Camera> camera) {
    /// Update all components
    position = camera->position;
    // render_comp.update(delta)
}
