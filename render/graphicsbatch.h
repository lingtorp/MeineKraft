#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include "primitives.h"

class RenderComponent;

class GraphicsBatch {
public:
    GraphicsBatch(uint64_t hash_id);

    uint64_t hash_id;
    Mesh mesh;
    std::vector<RenderComponent *> components;

    uint32_t gl_VAO;
    uint32_t gl_models_buffer_object;
    uint32_t gl_camera_view;
    ShaderType shader_program;
};

#endif //MEINEKRAFT_GRAPHICSBATCH_H
