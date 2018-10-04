#include "skybox.h"
#include "../render/camera.h"
#include "../render/rendercomponent.h"
#include "../util/filesystem.h"

Skybox::Skybox(): Entity() {
  scale = 100.0;
  auto render_comp = new RenderComponent(this);
  render_comp->set_mesh(MeshPrimitive::Cube);
  std::vector<std::string> faces = {Filesystem::base + std::string("resources/sky/right.jpg"),
                                    Filesystem::base + std::string("resources/sky/left.jpg"),
                                    Filesystem::base + std::string("resources/sky/top.jpg"),
                                    Filesystem::base + std::string("resources/sky/bottom.jpg"),
                                    Filesystem::base + std::string("resources/sky/back.jpg"),
                                    Filesystem::base + std::string("resources/sky/front.jpg")};
  render_comp->set_cube_map_texture(faces);
  attach_component(render_comp);
}

void Skybox::update(const uint64_t delta, const Camera& camera) {
  position = camera.position;
}
