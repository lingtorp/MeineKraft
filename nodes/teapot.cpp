#include "teapot.h"

static std::string file = "/Users/AlexanderLingtorp/Downloads/dragon.obj";

Teapot::Teapot(): Entity(generate_entity_id()), render_comp(RenderComponent(this, file)) {
    // /Users/AlexanderLingtorp/Downloads/dragon.obj
    // "/Users/AlexanderLingtorp/Downloads/teapot/teapot.obj"
    // "/Users/AlexanderLingtorp/Downloads/dabrovic-sponza/sponza.obj"
    scale = 20;
    render_comp.set_cube_map_texture(Texture::GRASS);
}
