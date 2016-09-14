#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include <SDL2/SDL_opengl.h>
#include "../math/vector.h"

/// The renderer shall compile all the shader sources at startup and then keep them in order to assigned the compiled
/// shader program to the correct GS whenever a GS is needed or created.

/// Give each Entity a key_name which hashes to a GraphicsState in the Renderer.
/// The renderer checks if the GS exists and if it does not creates it by creating all the
/// GLObjects and compiles the shaders and all that

/// All the GraphicsObjects that are known beforehand are registered in main with the render
/// the renderer uses this information in order to prepare as much of the GSs as possible

class GraphicsState {
    bool geometry_loaded = false;
    GLuint gl_VAO;
    GLuint gl_modelsBO;
    GLuint gl_camera_view;
    GLuint gl_shader_program;
};

class GraphicsEntity {
    Vec3<> position;
    std::shared_ptr<GraphicsState> graphics_state;
};

#endif //MEINEKRAFT_ENTITY_H
