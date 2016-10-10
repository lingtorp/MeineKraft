#include "rendercomponent.h"
#include "render.h"

RenderComponent::RenderComponent(Entity *entity, std::string mesh_file): entity(entity), graphics_state{}, id{0} {
    auto mesh = Renderer::instance().load_mesh_from_file(mesh_file);
    id = Renderer::instance().add_to_batch(this, mesh);
};

RenderComponent::RenderComponent(Entity *entity): entity(entity), graphics_state{}, id{0} {
    auto mesh = Cube();
    id = Renderer::instance().add_to_batch(this, mesh);
}

void RenderComponent::remove_component() {
    Renderer::instance().remove_from_batch(this);
}

void RenderComponent::set_cube_map_texture(Texture texture) {
    graphics_state.gl_texture = texture;
    // Render::instance().update_render_component(*this); // Super slow ..
}