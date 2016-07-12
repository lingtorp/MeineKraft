#include "render/render.h"

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window *window = SDL_CreateWindow(
            "", 0, 0, 1920, 1080,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    atexit(IMG_Quit);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG); // init sdl2_image

    Render render{window};
    Camera *camera = render.camera;

    World world{};

    bool DONE = false;
    uint32_t last_tick = SDL_GetTicks(), current_tick, delta;

    while (!DONE) {
        current_tick = SDL_GetTicks();
        delta = current_tick - last_tick;
        last_tick = current_tick;
        printf("Delta: %u \n", delta);

        // Process input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_MOUSEMOTION:
                    camera->pitch += event.motion.yrel;
                    camera->yaw += event.motion.xrel;
                    camera->direction = render.camera_direction(camera);
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_LEFT:
                            camera->yaw -= camera->movement_speed * delta;
                            camera->direction = render.camera_direction(camera);
                            break;
                        case SDLK_RIGHT:
                            camera->yaw += camera->movement_speed * delta;
                            camera->direction = render.camera_direction(camera);
                            break;
                        case SDLK_UP:
                            camera->pitch += camera->movement_speed * delta;
                            camera->direction = render.camera_direction(camera);
                            break;
                        case SDLK_DOWN:
                            camera->pitch -= camera->movement_speed * delta;
                            camera->direction = render.camera_direction(camera);
                            break;
                        case SDLK_w:
                            camera->position =
                                    vec_scalar_multiplication(render.camera_move_forward(camera), delta);
                            break;
                        case SDLK_a:
                            camera->position =
                                    vec_scalar_multiplication(render.camera_move_left(camera), delta);
                            break;
                        case SDLK_s:
                            camera->position =
                                    vec_scalar_multiplication(render.camera_move_backward(camera), delta);
                            break;
                        case SDLK_d:
                            camera->position =
                                    vec_scalar_multiplication(render.camera_move_right(camera), delta);
                            break;
                    }
                    break;
                case SDL_QUIT:
                    DONE = true;
                    break;
            }
        }

        // Tick/update the world
        world.world_tick(delta);

        // Render the world
        render.render_world(&world);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    return 0;
}
