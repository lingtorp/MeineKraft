#include "Skybox.h"
#include "../render/rendercomponent.h"

Skybox::Skybox(): Entity(), render_comp(RenderComponent{this}) {}

Skybox::~Skybox() {
    render_comp.remove_component();
}
