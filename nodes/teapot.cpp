#include "teapot.h"

static std::string mesh_file = "/Users/AlexanderLingtorp/Desktop/falu-stuga.obj";
static std::string directory_filepath = "/Users/AlexanderLingtorp/Desktop/";

Teapot::Teapot(): Entity(generate_entity_id()), render_comp(RenderComponent(this, mesh_file, directory_filepath)) {
    // /Users/AlexanderLingtorp/Downloads/dragon.obj
    // "/Users/AlexanderLingtorp/Downloads/teapot/teapot.obj"
    // "/Users/AlexanderLingtorp/Downloads/dabrovic-sponza/sponza.obj"
    scale = 1;
    // render_comp.set_cube_map_texture(Texture::GRASS);
}
