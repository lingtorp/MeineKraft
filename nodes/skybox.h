#ifndef MEINEKRAFT_SKYBOX_H
#define MEINEKRAFT_SKYBOX_H

#include "entity.h"

class Skybox: public Entity {
public:
  Skybox();

   void update(const uint64_t delta, const Camera& camera) override;
};

#endif // MEINEKRAFT_SKYBOX_H
