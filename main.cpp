#include "render/render.h"

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window *window = SDL_CreateWindow(
            "", 0, 0, 1000, 1000,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    atexit(IMG_Quit);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG); // init sdl2_image

    Render render{window};

    World world{};

    bool DONE = false;
    uint32_t last_tick = SDL_GetTicks(), current_tick, delta;

    while (!DONE) {
        current_tick = SDL_GetTicks();
        delta = current_tick - last_tick;
        last_tick = current_tick;
        printf("Delta: %u ms \n", delta);

        /// Process input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_MOUSEMOTION:
                    render.camera->pitch += event.motion.yrel;
                    render.camera->yaw += event.motion.xrel;
                    render.camera->direction = render.camera->recalculate_direction();
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                            render.camera->position = render.camera->move_forward(delta);
                            break;
                        case SDLK_a:
                            render.camera->position = render.camera->move_left(delta);
                            break;
                        case SDLK_s:
                            render.camera->position = render.camera->move_backward(delta);
                            break;
                        case SDLK_d:
                            render.camera->position = render.camera->move_right(delta);
                            break;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                            render.update_projection_matrix();
                            break;
                    }
                    break;
                case SDL_QUIT:
                    DONE = true;
                    break;
            }
        }

        /// Tick/update the world
        world.world_tick(delta, render.camera);

        /// Render the world
        render.render_world(&world);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    return 0;
}
