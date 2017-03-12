#include "block.h"

Block::Block(): Entity() {
    auto render_comp = new RenderComponent(this);
    render_comp->set_mesh(MeshPrimitive::Cube);
    const auto base = "/Users/AlexanderLingtorp/Repositories/MeineKraft/";
    std::vector<std::string> faces = {base + std::string("res/blocks/grass/side.jpg"),
                                      base + std::string("res/blocks/grass/side.jpg"),
                                      base + std::string("res/blocks/grass/top.jpg"),
                                      base + std::string("res/blocks/grass/bottom.jpg"),
                                      base + std::string("res/blocks/grass/side.jpg"),
                                      base + std::string("res/blocks/grass/side.jpg")};
    render_comp->set_cube_map_texture(faces);
    attach_component(render_comp);
}