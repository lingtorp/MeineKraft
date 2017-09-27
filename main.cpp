#include <memory>
#include "render/render.h"
#include "include/imgui/imgui_impl_sdl.h"
#include <SDL2/SDL_image.h>
#include "render/camera.h"
#include "world/world.h"
#include "nodes/skybox.h"
#include "nodes/model.h"

struct Resolution {
  int width, height;
};

static auto HD      = Resolution{1280, 720};
static auto FULL_HD = Resolution{1920, 1080};

int main() {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE;
  SDL_Window *window = SDL_CreateWindow("MeineKraft", 0, 0, HD.width, HD.height, window_flags);
  SDL_GLContext context = SDL_GL_CreateContext(window);

  // Init sdl2_image
  atexit(IMG_Quit);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  
  // Init ImGui
  ImGui_ImplSdlGL3_Init(window);

  // Inits glew
  Renderer& renderer = Renderer::instance();
  renderer.window = window;
  renderer.update_projection_matrix(70);

  // Init the world with seed
  World world{1};
  // Skybox skybox{};

  std::string directory = "/Users/AlexanderLingtorp/Desktop/";
  std::string model_file;
  
  model_file = "stanford-bunny.obj";
  Model bunny{model_file, directory};
  bunny.position = {0, 5, 0};
  bunny.scale = 100;

  model_file = "stanford-dragon.obj";
  Model dragon{model_file, directory};
  dragon.position = {18, 5, 0};
  dragon.scale = 2;
  
  bool toggle_mouse_capture = false;
  bool DONE = false;
  uint32_t last_tick = SDL_GetTicks(), current_tick, delta;

  while (!DONE) {
      current_tick = SDL_GetTicks();
      delta = current_tick - last_tick;
      last_tick = current_tick;
      // SDL_Log("Delta: %u ms \n", delta);

      /// Process input
      SDL_Event event{};
      while (SDL_PollEvent(&event)) {
        ImGui_ImplSdlGL3_ProcessEvent(&event);
      switch (event.type) {
        case SDL_MOUSEMOTION:
          if (toggle_mouse_capture) { break; }
          renderer.camera->pitch += event.motion.yrel;
          renderer.camera->yaw   += event.motion.xrel;
          renderer.camera->direction = renderer.camera->recalculate_direction();
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_w:
              renderer.camera->move_forward(true);
              break;
            case SDLK_a:
              renderer.camera->move_left(true);
              break;
            case SDLK_s:
              renderer.camera->move_backward(true);
              break;
            case SDLK_d:
              renderer.camera->move_right(true);
              break;
            case SDLK_q:
              renderer.camera->move_down(true);
              break;
            case SDLK_e:
              renderer.camera->move_up(true);
              break;
            case SDLK_TAB:
              toggle_mouse_capture = !toggle_mouse_capture;
              break;
            case SDLK_ESCAPE:
              DONE = true;
              break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_w:
              renderer.camera->move_forward(false);
              break;
            case SDLK_a:
              renderer.camera->move_left(false);
              break;
            case SDLK_s:
              renderer.camera->move_backward(false);
              break;
            case SDLK_d:
              renderer.camera->move_right(false);
              break;
            case SDLK_q:
              renderer.camera->move_down(false);
              break;
            case SDLK_e:
              renderer.camera->move_up(false);
          }
        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
              renderer.update_projection_matrix(70);
              break;
          }
          break;
        case SDL_QUIT:
          DONE = true;
          break;
      }
    }

    renderer.camera->position = renderer.camera->update(delta);

    /// Tick/update the world
    world.world_tick(delta, renderer.camera);

    /// Render the world
    renderer.render(delta);

    /// ImGui - Debug instruments
    ImGui_ImplSdlGL3_NewFrame(window);
    {
      auto io = ImGui::GetIO();

      static bool show_test_window;

      ImGui::Begin("Render state");
      // ImGui::Text("Chunks: %lu", world.chunks.size());
      ImGui::Text("Graphics batches: %llu", renderer.state.graphic_batches);
      ImGui::Text("Entities: %llu", renderer.state.entities);
      ImGui::Text("Application average %u ms / frame (%.1f FPS)", delta, io.Framerate);
      if (ImGui::Button("ImGui Palette")) show_test_window ^= 1;
      ImGui::End();

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
  return EXIT_SUCCESS;
}
