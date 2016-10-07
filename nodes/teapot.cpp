#include "teapot.h"

Teapot::Teapot(): Entity(generate_entity_id()), render_comp(RenderComponent(this, "/Users/AlexanderLingtorp/Downloads/teapot/teapot.obj")) {

}

void Teapot::update(uint64_t delta, const std::shared_ptr<Camera> camera) {

}

Teapot::~Teapot() {

}


