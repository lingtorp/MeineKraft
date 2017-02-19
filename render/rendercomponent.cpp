#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"

RenderComponent::RenderComponent(Entity *entity): entity(entity), graphics_state{} {}

void RenderComponent::set_obj_mesh(std::string mesh_file, std::string directory_file) {
    // TODO: Remove from previous batch - since we are chaning mesh and thus geo. data
    auto mesh_id = Renderer::instance().load_mesh(mesh_file, directory_file);
    Renderer::instance().add_to_batch(this, mesh_id);
};

void RenderComponent::update() {
    // TODO: Copy entity positional data to graphics_state
    graphics_state.scale = entity->scale;
    graphics_state.position = entity->position;
};
