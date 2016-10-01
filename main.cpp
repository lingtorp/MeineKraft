#include "render/render.h"
#include "include/imgui/imgui.h"
#include "include/imgui/imgui_impl_sdl.h"

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE;
    SDL_Window *window = SDL_CreateWindow("MeineKraft", 0, 0, 1000, 1000, window_flags);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    atexit(IMG_Quit);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG); // init sdl2_image

    // Init ImGui
    ImGui_ImplSdlGL3_Init(window);

    // Inits glew
    Render render{window};

    // Init the world with seed
    World world{1};

    bool DONE = false;
    uint32_t last_tick = SDL_GetTicks(), current_tick, delta;

    while (!DONE) {
        current_tick = SDL_GetTicks();
        delta = current_tick - last_tick;
        last_tick = current_tick;
        SDL_Log("Delta: %u ms \n", delta);

        /// Process input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
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

        ImGui_ImplSdlGL3_NewFrame(window);

        /// Tick/update the world
        world.world_tick(delta, render.camera);

        /// Render the world
        render.render_world(&world);

        /// ImGui - Debug instruments
        {
            auto clear_color = ImColor(114, 144, 154);
            auto show_test_window = true;
            auto show_another_window = true;
            // 1. Show a simple window
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
            {
                static float f = 0.0f;
                ImGui::Text("Hello, world!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                ImGui::ColorEdit3("clear color", (float *) &clear_color);
                if (ImGui::Button("Test Window")) show_test_window ^= 1;
                if (ImGui::Button("Another Window")) show_another_window ^= 1;
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                            ImGui::GetIO().Framerate);
            }

            // 2. Show another simple window, this time using an explicit Begin/End pair
            if (show_another_window) {
                ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
                ImGui::Begin("Another Window", &show_another_window);
                ImGui::Text("Hello");
                ImGui::End();
            }

            // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
            if (show_test_window) {
                ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
                ImGui::ShowTestWindow(&show_test_window);
            }
        }

        ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
