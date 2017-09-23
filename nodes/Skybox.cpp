#include "skybox.h"
#include "../render/camera.h"
#include "../render/rendercomponent.h"

Skybox::Skybox(): Entity() {
    scale = 100;
    auto render_comp = new RenderComponent(this);
    render_comp->set_mesh(MeshPrimitive::Cube);
    const auto base = "/Users/AlexanderLingtorp/Repositories/MeineKraft/";
    std::vector<std::string> faces = {base + std::string("res/sky/right.jpg"),
                                      base + std::string("res/sky/left.jpg"),
                                      base + std::string("res/sky/top.jpg"),
                                      base + std::string("res/sky/bottom.jpg"),
                                      base + std::string("res/sky/back.jpg"),
                                      base + std::string("res/sky/front.jpg")};
    render_comp->set_cube_map_texture(faces);
    attach_component(render_comp);
}

void Skybox::update(uint64_t delta, const std::shared_ptr<Camera> &camera) {
    position = camera->position;
}
