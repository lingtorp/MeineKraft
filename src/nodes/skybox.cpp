#include "skybox.hpp"
#include "../rendering/camera.hpp"
#include "../rendering/rendercomponent.hpp"
#include "../util/filesystem.hpp"
#include "../rendering/renderer.hpp"
#include "transform.hpp"

Skybox::Skybox(): Entity() {
  NameSystem::instance().add_name_to_entity("Skybox", this->id);
  TransformComponent transform;
  transform.scale = 50.0;
  attach_component(transform);
  RenderComponent render;
  render.set_mesh(MeshPrimitive::CubeCounterClockWinding);
  std::array<std::string, 6> faces = {Filesystem::base + std::string("resources/environmentmaps/garden/negx.bmp"),
                                      Filesystem::base + std::string("resources/environmentmaps/garden/posx.bmp"),
                                      Filesystem::base + std::string("resources/environmentmaps/garden/posy.bmp"),
                                      Filesystem::base + std::string("resources/environmentmaps/garden/negy.bmp"),
                                      Filesystem::base + std::string("resources/environmentmaps/garden/posz.bmp"),
                                      Filesystem::base + std::string("resources/environmentmaps/garden/negz.bmp")};
  render.set_cube_map_texture(faces);
  render.set_shading_model(ShadingModel::Unlit);
  attach_component(render);

  faces = {Filesystem::base + std::string("resources/lightmaps/garden/negx.bmp"),
           Filesystem::base + std::string("resources/lightmaps/garden/posx.bmp"),
           Filesystem::base + std::string("resources/lightmaps/garden/posy.bmp"),
           Filesystem::base + std::string("resources/lightmaps/garden/negy.bmp"),
           Filesystem::base + std::string("resources/lightmaps/garden/posz.bmp"),
           Filesystem::base + std::string("resources/lightmaps/garden/negz.bmp")};
  MeineKraft::instance().renderer->load_environment_map(faces);
}
