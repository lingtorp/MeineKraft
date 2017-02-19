#include "graphicsbatch.h"
#include "rendercomponent.h"

GraphicsBatch::GraphicsBatch(uint64_t mesh_id): mesh_id(mesh_id), components{}, mesh{}, gl_camera_view(0),
                                                gl_models_buffer_object(0), shader_type(ShaderType::STANDARD_SHADER), gl_VAO(0) {};