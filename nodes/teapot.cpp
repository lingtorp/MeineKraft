#include "teapot.h"

Teapot::Teapot(std::string mesh_file, std::string directory): Entity() {
    auto render_comp = new RenderComponent(this);
    render_comp->set_mesh(mesh_file, directory);
    attach_component(render_comp);
    scale = 1;
}


