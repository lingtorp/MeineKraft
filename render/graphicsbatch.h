#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include "primitives.h"

class RenderComponent;

class GraphicsBatch {
public:
    GraphicsBatch(uint64_t hash_id);

    uint64_t hash_id;
    Mesh mesh;
    std::vector<RenderComponent> components;

    uint64_t gl_VAO;
    uint64_t gl_models_buffer_object;
    uint64_t gl_camera_view;
    uint64_t gl_shader_program;
};

#endif //MEINEKRAFT_GRAPHICSBATCH_H
