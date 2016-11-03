#include "block.h"

Block::Block(): Entity(generate_entity_id()), render_comp{this} {
//    render_comp.set_cube_map_texture(Texture::GRASS);
}

Block::~Block() {
    render_comp.remove_component();
}

void Block::update(uint64_t delta, const std::shared_ptr<Camera> camera) {
    // TODO: Implement
}
