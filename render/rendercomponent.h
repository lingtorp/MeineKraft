#ifndef MEINEKRAFT_RENDERCOMPONENT_H
#define MEINEKRAFT_RENDERCOMPONENT_H

#include "primitives.h"

class Entity;

class Component {
public:
  /// Called when the component is added to the Entity
  virtual void did_attach_to_entity(Entity* entity) {}
  /// Called when the component is removed to the Entity
  virtual void did_deattach_from_entity(Entity* entity) {}
  /// Called once every frame
  virtual void update() {}
};

class RenderComponent: public Component {
public:
  GraphicsState graphics_state;
  Entity* entity;

  /// Creates a RenderComponent with the mesh of a .obj file
  explicit RenderComponent(Entity* entity);

  /// Sets the mesh for the RenderComponent from the .obj file in directory_file
  void set_mesh(const std::string& directory, const std::string& file);

  /// Sets the mesh to one of the Mesh primitives
  void set_mesh(MeshPrimitive primitive);

  /** Component Interface **/
  void update() override;

  void did_attach_to_entity(Entity* entity) override;

  /// Sets the cube map texture to the bounded mesh
  /// order; right, left, top, bot, back, front
  void set_cube_map_texture(const std::vector<std::string>& faces);
};

#endif // MEINEKRAFT_RENDERCOMPONENT_H
