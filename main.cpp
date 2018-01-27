#include <memory>
#include "render/render.h"
#include "include/imgui/imgui_impl_sdl.h"
#include <SDL2/SDL_image.h>
#include "render/camera.h"
#include "world/world.h"
#include "nodes/skybox.h"
#include "nodes/model.h"
#include "util/filesystem.h"

#define MK_ARRAYSIZE(_ARR) ((int) (sizeof(_ARR)/sizeof(*_ARR)))

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
  auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE;
  SDL_Window* window = SDL_CreateWindow("MeineKraft", 0, 0, HD.width, HD.height, window_flags);
  SDL_GLContext context = SDL_GL_CreateContext(window);
  SDL_GL_SetSwapInterval(0); // Disables vsync

  // Init sdl2_image
  atexit(IMG_Quit);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  
  // Init ImGui
  ImGui_ImplSdlGL3_Init(window);
  
  // Inits glew
  Renderer& renderer = Renderer::instance();
  renderer.window = window;
  renderer.update_projection_matrix(70);
  renderer.camera->position = {-0.62f, 17.0f, 2.6f};
  renderer.screen_width = HD.width;
  renderer.screen_height = HD.height;
  
  Model bunny{FileSystem::base, "stanford-bunny.obj"};
  bunny.position = {0, 15, 0};
  
  bool toggle_mouse_capture = true;
  bool DONE = false;
  uint32_t last_tick = SDL_GetTicks(), current_tick, delta;
  
  /// Delta values
  float deltas[100];
  
  while (!DONE) {
      current_tick = SDL_GetTicks();
      delta = current_tick - last_tick;
      last_tick = current_tick;

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

    /// Render the world
    renderer.render(delta);

    /// ImGui - Debug instruments
    ImGui_ImplSdlGL3_NewFrame(window);
    {
      auto io = ImGui::GetIO();

      ImGui::Begin("Render state");
      ImGui::Text("Graphics batches: %llu", renderer.state.graphic_batches);
      ImGui::Text("Entities: %llu", renderer.state.entities);
      ImGui::Text("Application average %u ms / frame (%.1f FPS)", delta, io.Framerate);
  
      static int i = -1; i = (i + 1) % MK_ARRAYSIZE(deltas);
      deltas[i] = delta;
      ImGui::PlotLines("", deltas, 100, 0, "ms / frame", 0.0f, 50.0f, ImVec2(ImGui::GetWindowWidth(), 100));
      
      static bool show_test_window = false;
      if (ImGui::Button("ImGui Palette")) show_test_window ^= 1;
      if (show_test_window) {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
      }
      
      if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputFloat3("Position", &renderer.camera->position.x);
        ImGui::InputFloat3("Direction", &renderer.camera->direction.x);
      }
      
      if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputInt("Samples", (int*) &renderer.ssao_num_samples, 8, 16);
        ImGui::InputFloat("Kernel radius", &renderer.ssao_kernel_radius, 0.1f, 0.2f);
        ImGui::InputFloat("Effect power", &renderer.ssao_power, 0.1f, 0.2f);
        ImGui::InputFloat("Bias", &renderer.ssao_bias, 0.0001f, 0.0005f);
        ImGui::Checkbox("Blur Enabled", &renderer.ssao_blur_enabled);
        ImGui::InputFloat("Blur factor", &renderer.ssao_blur_factor, 0.5f, 1.0f);
      }
      
      if (ImGui::CollapsingHeader("Lightning", 0)) {
        ImGui::Checkbox("Enable lightning", &renderer.lightning_enabled);
        ImGui::InputFloat3("Color (RGB)", &renderer.lights[0].light_color.r);
        ImGui::InputFloat3("Intensity (ambient, diffuse, specular)", &renderer.lights[0].light_itensity.x);
      }
      
      ImGui::End();
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
