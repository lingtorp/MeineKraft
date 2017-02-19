#include "block.h"

Block::Block(): Entity(), render_comp{this} {
//    render_comp.set_cube_map_texture(Texture::GRASS);
}

Block::~Block() {
    // TODO: Dealloc RenderComp
}

void Block::update(uint64_t delta, const std::shared_ptr<Camera> camera) {
    // TODO: Implement
}
