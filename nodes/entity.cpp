#include "entity.h"

Entity::~Entity() {
    for (auto &comp : components) { comp->did_deattach_from_entity(this); }
}
