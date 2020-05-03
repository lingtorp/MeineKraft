#include "meinekraft.hpp"

#include <chrono>
#include <SDL2/SDL_events.h>

#include "imgui/imgui.h"
#include "../include/imgui/imgui_impl_sdl.h"

#include "rendering/primitives.hpp"
#include "rendering/renderer.hpp"
#include "rendering/camera.hpp"
#include "rendering/debug_opengl.hpp"
#include "nodes/model.hpp"
#include "nodes/skybox.hpp"
#include "nodes/physics_system.hpp"
#include "scene/world.hpp"
#include "rendering/graphicsbatch.hpp"
#include "util/filesystem.hpp"
#include "util/config.hpp"
#include "util/logging_system.hpp"
#include "util/mkass.hpp"
// #include "network/network_system.hpp"

// TODO: Try to update ImGui some day

// ImGui global GUI settings
struct {
  bool imgui_test_window = false;    // ImGui demonstration window
  bool render_system_window = true;  // Render system information and config. window
  bool scene_graph_window = true;    // Scene graph of Graphic entities
  bool logger_window = true;         // TODO: Logger of engine messages
  bool console_window = true;        // TODO: Console for engine commands
  bool network_window = false;       // TODO: Network system
  bool scripting_window = true;      // Scripting window for writing small programs
  bool help_window = false;          // TODO: Helpful keyboard shortcuts, displayed on first launch
  bool about_window = false;         // TODO: Displays some information about the application
} Gui;

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.txt)
static void ImGui_HelpMarker(const std::string& str) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(str.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

/// Used for the Render system frame time graph
static ImVec4 imgui_color_for_value(const float v, const float a, const float b) {
  ImVec4 color;
 
  if (v <= a) {
    color = ImVec4(0.35f, 1.0f, 0.0f, 0.57f); // Good
  } else if (v <= b) {
    color = ImVec4(1.0f, 1.0f, 0.0f, 0.57f); // OK
  } else {
    color = ImVec4(1.0, 0.35f, 0.0f, 0.57f); // Bad
  }

  return color;
}

/// Sets the custom engine styling for ImGui
void imgui_styling() {
  ImGuiStyle* style = &ImGui::GetStyle();

  style->WindowPadding = ImVec2(15, 15);
  style->WindowRounding = 5.0f;
  style->WindowTitleAlign = ImVec2(0.5, 0.5);
  style->FramePadding = ImVec2(5, 5);
  style->FrameRounding = 4.0f;
  style->ItemSpacing = ImVec2(12, 8);
  style->ItemInnerSpacing = ImVec2(10, 6);
  style->IndentSpacing = 25.0f;
  style->ScrollbarSize = 15.0f;
  style->ScrollbarRounding = 9.0f;
  style->GrabMinSize = 20.0f;
  style->GrabRounding = 3.0f;

  style->AntiAliasedLines = true;
  style->AntiAliasedShapes = true;
  style->FrameRounding = true;

  style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
  style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 0.80f);
  style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
  style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
  style->Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
  style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.19f, 0.25f, 1.00f);
  style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.40f, 0.75f);
  style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
  style->Colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 1.0f, 0.40f, 0.31f);
  style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
  style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
  style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
  style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
  style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
  style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.88f, 0.63f);
  style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 1.00f, 1.0f, 1.00f);
  style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
  style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 1.00f, 1.0f, 1.00f);
  style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.55f, 0.55f, 0.55f, 0.43f);
  style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
}

static void dump_performance_data(const struct Renderer* render, const size_t fps) {
  const float total_time = render->state.total_execution_time / 1'000'000'00.0f;

  float OH = 0.0f;
  float GI = 0.0f;
  float GB = 0.0f;
  float DS = 0.0f;

  nlohmann::json obj;

  // General information
  {
    obj["general"]["fps"] = fps;
  }

  // GPU view frustum culling
  if (render->state.culling.enabled) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.culling.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["culling"]["time_ms"] = time;
    obj["culling"]["time_percent"] = time / total_time;
    OH += time / total_time;
  }

  // Lighting application
  {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.lighting.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["lighting_application"]["time_ms"] = time;
    obj["lighting_application"]["time_percent"] = time / total_time;
    OH += time / total_time;
  }

  // Gbuffer generation
  {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.gbuffer.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["gbuffer"]["time_ms"] = time;
    obj["gbuffer"]["time_percent"] = time / total_time;
    GI += time / total_time;
  }

  // Gbuffer downsample pass
  if (render->state.lighting.downsample_modifier > 1) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.gbuffer_downsample.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["gbuffer_downsample"]["time_ms"] = time;
    obj["gbuffer_downsample"]["time_percent"] = time / total_time;
    GI += time / total_time;
  }

  // Voxelization pass
  if (render->state.voxelization.always_voxelize) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.voxelization.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["voxelization"]["time_ms"] = time;
    obj["voxelization"]["time_percent"] = time / total_time;
    GI += time / total_time;
  }

  // VCT
  {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.vct.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["vct"]["time_ms"] = time;
    obj["vct"]["time_percent"] = time / total_time;
    GI += time / total_time;
  }

  if (render->state.shadow.enabled) {
    float time0 = 0.0f;
    float time1 = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time0 += render->state.shadow.execution_time_shadowmapping[i] / 1'000'000.0f;
      time1 += render->state.shadow.execution_time_shadow[i] / 1'000'000.0f;
    }
    time0 /= RenderState::execution_time_buffer_size;
    time1 /= RenderState::execution_time_buffer_size;
    obj["depth_map_generation"]["time_ms"] = time0;
    obj["depth_map_generation"]["time_percent"] = time0 / total_time;
    obj["shadow_application"]["time_ms"] = time1;
    obj["shadow_application"]["time_percent"] = time1 / total_time;
    GI += (time0 + time1) / total_time;
  }

  // Bilateral filtering pass
  if (render->state.bilateral_filtering.enabled && render->state.lighting.downsample_modifier > 1) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.bilateral_filtering.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["bilateral_filtering"]["time_ms"] = time;
    obj["bilateral_filtering"]["time_percent"] = time / total_time;
  }

  // Bilateral upsampling pass
  if (render->state.bilateral_upsample.enabled && render->state.lighting.downsample_modifier > 1) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.bilateral_upsample.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["bilateral_upsampling"]["time_ms"] = time;
    obj["bilateral_upsampling"]["time_percent"] = time / total_time;
  }

  // Bilinear upsampling pass
  if (render->state.bilinear_upsample.enabled && render->state.lighting.downsample_modifier > 1) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.bilinear_upsample.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["bilinear_upsampling"]["time_ms"] = time;
    obj["bilinear_upsampling"]["time_percent"] = time / total_time;
  }

  // Voxel visualization pass
  if (render->state.voxel_visualization.enabled) {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.voxel_visualization.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["voxel_visualization"]["time_ms"] = time;
    obj["voxel_visualization"]["time_percent"] = time / total_time;
  }

  // Blit
  {
    float time = 0.0f;
    for (size_t i = 0; i < RenderState::execution_time_buffer_size; i++) {
      time += render->state.blit.execution_time[i] / 1'000'000.0f;
    }
    time /= RenderState::execution_time_buffer_size;
    obj["blit"]["time_ms"] = time;
    obj["blit"]["time_percent"] = time / total_time;
    OH += time / total_time;
  }

  // OH + GI
  {
    obj["OH"]["time_percentage"] = OH;
    obj["GI"]["time_percentage"] = GI;
  }

  Filesystem::save_text_in_file(Filesystem::tmp + "performance_data", obj.dump(2));
}

void record_fps_loop(struct Renderer* renderer) {
  const auto component = ActionComponent([=](const uint64_t frame,
                                             const uint64_t delta) -> bool {
        static auto step = 0;
        static const auto MEASUREMENTS = 90;
        static std::array<uint32_t, MEASUREMENTS> fps;

        if (step == 0) {
          renderer->scene->camera.position = Vec3f(-562.0f, 583.0f, -9.0f);
          renderer->scene->camera.direction = Vec3f(0.64f, -0.3f, -0.71f);
        }
        else if (step == 29) {
          renderer->scene->camera.position = Vec3f(-956.0f, 837.0f, -505.0f);
          renderer->scene->camera.direction = Vec3f(0.66f, -0.61f, 0.42f);
        }
        else if (step == 59) {
          renderer->scene->camera.position = Vec3f(1123.0f, 182.0f, -101.0f);
          renderer->scene->camera.direction = Vec3f(-1.0f, -0.1f, 0.0f);
        }

        fps[step] = 1'000.0f / delta;
        renderer->scene->camera.position += Vec3f(0.05f) * step;

        step++;

        if (step == MEASUREMENTS) {
          std::string txt;
          float avg_fps = 0.0f;
          for (size_t i = 0; i < fps.size(); i++) {
            txt += "(" + std::to_string(i) + ", " + std::to_string(fps[i]) + ") \n";
            avg_fps += fps[i];
          }
          avg_fps /= fps.size();
          txt += "\n AVG FPS: " + std::to_string(avg_fps) + "\n";
          Filesystem::save_text_in_file(Filesystem::tmp + "fps", txt);
        }

        return step == MEASUREMENTS;
      });
  ActionSystem::instance().add_component(component);
}

void record_screenshot_loop(struct Renderer* renderer) {

}

/// Main engine constructor
MeineKraft::MeineKraft() {
  // TODO: Enable configuration to set windows position, size and whether or not to be centered
  const Resolution res = FULL_HD;

  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_MOUSE_CAPTURE;
  window = SDL_CreateWindow("MeineKraft", 0, 0, res.width, res.height, window_flags);
  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) { Log::error(std::string(SDL_GetError())); }
  SDL_GL_SetSwapInterval(0); // Disables vsync

  glewExperimental = (GLboolean) true;
  glewInit();

  // Create MeineKraft folder structure
  Filesystem::create_directory(Filesystem::tmp);

  OpenGLContextInfo gl_context_info(4, OPENGL_MINOR_VERSION);

  atexit(IMG_Quit);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG); 
  
  ImGui_ImplSdlGL3_Init(window);

  renderer = new Renderer(res);

  imgui_styling();
}

void MeineKraft::init() {
  bool success = false;
  const auto config = Config::load_config(success);
  if (success) {
    const std::string path = config["scene"]["path"].get<std::string>();
    const std::string name = config["scene"]["name"].get<std::string>();
    renderer->scene = new Scene(Filesystem::home + path, name);
    renderer->scene->camera = Camera(config);
  } else {
    renderer->scene = new Scene();
    Log::warn("Failed to load config.json.");
  }

  screenshot_mode = config["screenshot_mode"].get<bool>();

  renderer->init();
  LoggingSystem::instance().init();
  MkAssProgramManager::instance().init();
}

MeineKraft::~MeineKraft() {
  if (renderer) { delete renderer; }
  ImGui_ImplSdlGL3_Shutdown();
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void MeineKraft::mainloop() {
  World world;

  bool toggle_mouse_capture = true;
  bool done = false;
  auto last_tick = std::chrono::high_resolution_clock::now();
  auto current_tick = last_tick;
  int64_t delta_ms = 0;
  int64_t delta_ns = 0;

  /// Delta values
  const int num_deltas = 150;
  float deltas_ms[num_deltas];
  bool take_screenshot = false;

  /// Window handling
  bool throttle_rendering = false;
  bool throttle_rendering_enabled = false; // NOTE: Enables the flag 'throttle_rendering'
  bool keyboard_enabled = true;

  while (!done) {
      current_tick = std::chrono::high_resolution_clock::now();
      delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_tick - last_tick).count();
      delta_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_tick - last_tick).count();
      last_tick = current_tick;

      /// Process input
      SDL_Event event{};
      while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSdlGL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_MOUSEMOTION:
          if (toggle_mouse_capture) { break; }
          renderer->scene->camera.pitch += event.motion.yrel;
          renderer->scene->camera.yaw += event.motion.xrel;
          renderer->scene->camera.direction = renderer->scene->camera.recalculate_direction();
          break;

        case SDL_MOUSEBUTTONDOWN:
          if (!ImGui::IsMouseHoveringAnyWindow()) {
            world.spawn_entity(MeshPrimitive::Sphere, renderer->scene->camera.position, renderer->scene->camera.direction, 20.0f);
          }
          break;

        case SDL_KEYDOWN:
          if (!keyboard_enabled) {
            break;
          }
          switch (event.key.keysym.sym) {
          case SDLK_w:
            renderer->scene->camera.move_forward(true);
            break;
          case SDLK_a:
            renderer->scene->camera.move_left(true);
            break;
          case SDLK_s:
            renderer->scene->camera.move_backward(true);
            break;
          case SDLK_d:
            renderer->scene->camera.move_right(true);
            break;
          case SDLK_q:
            renderer->scene->camera.move_down(true);
            break;
          case SDLK_e:
            renderer->scene->camera.move_up(true);
            break;
          case SDLK_TAB:
            toggle_mouse_capture = !toggle_mouse_capture;
            break;
          case SDLK_ESCAPE:
            done = true;
            break;
          }
          break;

        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
          case SDLK_w:
            renderer->scene->camera.move_forward(false);
            break;
          case SDLK_a:
            renderer->scene->camera.move_left(false);
            break;
          case SDLK_s:
            renderer->scene->camera.move_backward(false);
            break;
          case SDLK_d:
            renderer->scene->camera.move_right(false);
            break;
          case SDLK_q:
            renderer->scene->camera.move_down(false);
            break;
          case SDLK_e:
            renderer->scene->camera.move_up(false);
          }
          break;

        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
              Resolution screen;
              SDL_GetWindowSize(window, &screen.width, &screen.height);
              // TODO: Support screen resizing?
              break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
              throttle_rendering = false;
              break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
              throttle_rendering = true; // FIXME: May cause problems when RenderDoc/Nsight is attached
              break;
          }
          break;

        case SDL_QUIT:
          done = true;
          break;
      }
    }
    renderer->scene->camera.position = renderer->scene->camera.update(delta_ms);

    /// Run all actions
    ActionSystem::instance().execute_actions(renderer->state.frame, delta_ms);

    /// Let the game do its thing
    world.tick();

    /// Physics computations
    PhysicsSystem::instance().update_system(delta_ms);

    /// Render the world
    if (!throttle_rendering || !throttle_rendering_enabled) {
      renderer->render(delta_ms);
    }

    if (screenshot_mode && renderer->state.frame > screenshot_mode_frame_wait) {
      take_screenshot = true;
    }

    if (take_screenshot) {
      const Vec3f *pixels = renderer->take_screenshot();
      Filesystem::save_image_as_ppm(Filesystem::tmp + "screenshot", pixels, renderer->screen.width, renderer->screen.height);
      delete pixels;
      take_screenshot = false;
    }

    if (screenshot_mode && renderer->state.frame > screenshot_mode_frame_wait) { return; }

    /// ImGui - Debug instruments
    if (!throttle_rendering || !throttle_rendering_enabled) {
      begin_gl_cmds("ImGui");

      ImGui_ImplSdlGL3_NewFrame(window);
      auto io = ImGui::GetIO();

      //  Main window
      {
        if (Gui.imgui_test_window) {
          ImGui::ShowTestWindow(); // NOTE: Shows demo of ImGui widgets
        }

        if (ImGui::BeginMainMenuBar()) {
          if (ImGui::BeginMenu("MeineKraft")) {
            if (ImGui::MenuItem("Quit", "ESC")) { done = true; }
            if (ImGui::MenuItem("Screenshot")) { take_screenshot = true; }
            if (ImGui::MenuItem("Pixel diff")) { renderer->state.bilateral_filtering.pixel_diff = true; }
            if (ImGui::MenuItem("Dump performance data")) { dump_performance_data(renderer, io.Framerate); }
            if (ImGui::MenuItem("Hide GUI")) { memset(&Gui, 0, sizeof(Gui)); }
            ImGui::EndMenu();
          }

          if (ImGui::BeginMenu("Windows")) {
            if (ImGui::MenuItem("Render system", "CTRL+R")) { Gui.render_system_window = !Gui.render_system_window; }
            if (ImGui::MenuItem("Logger system", "CTRL+L")) { Gui.logger_window        = !Gui.logger_window; }
            if (ImGui::MenuItem("Scene graph",   "CTRL+S")) { Gui.scene_graph_window   = !Gui.scene_graph_window; }
            if (ImGui::MenuItem("Scripting",     "CTRL+P")) { Gui.scripting_window     = !Gui.scripting_window; }
            if (ImGui::MenuItem("Console",       "CTRL+C")) { Gui.console_window       = !Gui.console_window; }
            ImGui::Separator();
            if (ImGui::MenuItem("Close all ..")) {  }; // TODO: Implement ...
            if (ImGui::MenuItem("Reset position ..")) {  }; // TODO: Implement ...
            ImGui::Separator();
            if (ImGui::MenuItem("ImGui demo")) { Gui.imgui_test_window = !Gui.imgui_test_window; }
            ImGui::EndMenu();
          }

          ImGui::Text("(%.1f FPS)", io.Framerate);

          ImGui::EndMainMenuBar();
        }

        // Render system
        if (Gui.render_system_window) {
          ImGui::Begin("Render system", &Gui.render_system_window);

          // NOTE: Execution time per render pass is only updated every 'execution_time_buffer_size'th time
          ImGui::PushItemWidth(90.0f);

          static size_t i = -1; i = (i + 1) % num_deltas;
          deltas_ms[i] = float(delta_ms);
          const ImVec4 color = imgui_color_for_value(float(delta_ms), 16.0f, 20.0f);
          ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
          ImGui::PlotLines("", deltas_ms, num_deltas, 0, "ms / frame", 0.0f, 50.0f, ImVec2(ImGui::GetWindowWidth(), 80));
          ImGui::PopStyleColor();

          // Measurements recording
          if (ImGui::Button("Record FPS loop")) {
            record_fps_loop(renderer);
          }

          if (ImGui::Button("Record screenshot loop")) {
            record_screenshot_loop(renderer);
          }

          ImGui::Text("Average %lu ms/frame (%.1f FPS)", delta_ms, io.Framerate);
          ImGui::Text("Frame: %lu", renderer->state.frame);
          ImGui::Text("Resolution: (%u, %u)", renderer->screen.width, renderer->screen.height);
          ImGui::Text("Render passes: %u", renderer->state.render_passes);
          ImGui::Text("Render passes total time (ms): %.1f", renderer->state.total_execution_time / 1'000'000.0f);
          // TODO: Change resolution, memory usage, textures, render pass execution times, etc

          if (ImGui::CollapsingHeader("Global settings")) {
            ImGui::Checkbox("Normal mapping", &renderer->state.lighting.normalmapping);
            ImGui::SliderInt("Downsample modifier", &renderer->state.lighting.downsample_modifier, 1, 8);
            ImGui::SameLine(); ImGui_HelpMarker("GI is performed in lower resolution by a of factor 1/x");

            ImGui::Checkbox("Capture render pass timings", &renderer->state.capture_execution_timings);
            ImGui::SameLine(); ImGui_HelpMarker("Performance timings updated or not");

            ImGui::Checkbox("Throttle rendering in background", &throttle_rendering_enabled);
            ImGui::SameLine(); ImGui_HelpMarker("Disables rendering when in the background.");

            if (ImGui::TreeNode("GPU view frustum culling")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.culling.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.culling.execution_time[0]) / float(renderer->state.total_execution_time));
              ImGui::Checkbox("Enabled##culling", &renderer->state.culling.enabled);
              ImGui::TreePop();
            }


            if (ImGui::TreeNode("Depth map generation")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.shadow.execution_time_shadowmapping[0] / 1'000'000.0f, 100.0f * float(renderer->state.shadow.execution_time_shadowmapping[0]) / float(renderer->state.total_execution_time));
              ImGui::TreePop();
            }

            if (ImGui::TreeNode("Global buffer generation")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.gbuffer.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.gbuffer.execution_time[0]) / float(renderer->state.total_execution_time));
              ImGui::TreePop();
              if (renderer->state.lighting.downsample_modifier > 1) {
                if (ImGui::TreeNode("Global buffer downsample pass")) {
                  ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.gbuffer_downsample.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.gbuffer_downsample.execution_time[0]) / float(renderer->state.total_execution_time));
                  ImGui::TreePop();
                }
              }
            }

            if (ImGui::TreeNode("Final blit pass")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.blit.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.blit.execution_time[0]) / float(renderer->state.total_execution_time));
              ImGui::TreePop();
            }
          }

          if (ImGui::CollapsingHeader("Direct shadows")) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.shadow.execution_time_shadow[0] / 1'000'000.0f, 100.0f * float(renderer->state.shadow.execution_time_shadow[0]) / float(renderer->state.total_execution_time));
            ImGui::Checkbox("Enabled", &renderer->state.lighting.direct);
            ImGui::Text("Shadowmap resolution: (%u, %u)", renderer->state.shadow.SHADOWMAP_W, renderer->state.shadow.SHADOWMAP_H);
            ImGui::SliderInt("Resolution modifier", &renderer->state.shadow.shadowmap_resolution_step, 1, 5);
            ImGui::SameLine(); ImGui_HelpMarker("Changes the shadowmap texture size");

            static int s = 0; // Selection
            ImGui::Combo("Algorithm", &s, "Plain \0 PCF \0 VCT \0");
            ImGui::SameLine(); ImGui_HelpMarker("Shadow algorithm applied by the directional light \n Plain: Classic shadowmapping \n PCF: Percentage-closer filtering \n VCT: Voxel cone traced shadows");
            renderer->state.shadow.algorithm = ShadowAlgorithm(s);

            switch (renderer->state.shadow.algorithm) {
            case ShadowAlgorithm::Plain:
                ImGui::InputFloat("Shadow bias", &renderer->state.shadow.bias);
                ImGui::SameLine(); ImGui_HelpMarker("Offset along normal to avoid shadow acne");
                break;
            case ShadowAlgorithm::PercentageCloserFiltering:
                ImGui::InputFloat("Shadow bias", &renderer->state.shadow.bias);
                ImGui::SameLine(); ImGui_HelpMarker("Offset along normal to avoid shadow acne");
                ImGui::InputInt("Samples", &renderer->state.shadow.pcf_samples);
                break;
            case ShadowAlgorithm::VCT:
                ImGui::InputFloat("Shadow cone aperature (deg.)", &renderer->state.shadow.vct_cone_aperature);
                break;
            }
          }

          if (ImGui::CollapsingHeader("Voxelization")) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.voxelization.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.voxelization.execution_time[0]) / float(renderer->state.total_execution_time));

            if (ImGui::Button("Voxelize")) {
              renderer->state.voxelization.voxelize = true;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Always voxelize", &renderer->state.voxelization.always_voxelize);

            ImGui::Checkbox("Conservative voxelization", &renderer->state.voxelization.conservative_rasterization);
          }

          if (ImGui::CollapsingHeader("Voxel cone tracing", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.vct.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.vct.execution_time[0]) / float(renderer->state.total_execution_time));

            ImGui::InputFloat("Ambient decay factor", &renderer->state.vct.ambient_decay);

            ImGui::Checkbox("Indirect", &renderer->state.lighting.indirect);
            ImGui::SameLine();
            ImGui::Checkbox("Specular", &renderer->state.lighting.specular);
            ImGui::SameLine();
            ImGui::Checkbox("Ambient", &renderer->state.lighting.ambient);
            ImGui::SameLine();
            ImGui_HelpMarker("Note ambient is only available when 'Indirect' is used.");

            if (ImGui::TreeNode("Diffuse cone settings")) {
              ImGui::InputFloat("Roughness aperature (deg.)", &renderer->state.vct.roughness_aperature);
              ImGui::SliderInt("# diffuse cones", &renderer->state.vct.num_diffuse_cones, 4, renderer->state.vct.MAX_DIFFUSE_CONES);
              ImGui::TreePop();
            }

            if (ImGui::TreeNode("Specular cone settings")) {
              ImGui::InputFloat("Metallic aperature (half ang. deg.)", &renderer->state.vct.metallic_aperature);
              if (ImGui::Button("1")) {
               renderer->state.vct.metallic_aperature = 1.0f;
              }
              if (ImGui::Button("2")) {
               renderer->state.vct.metallic_aperature = 2.0f;
              }
              if (ImGui::Button("3")) {
               renderer->state.vct.metallic_aperature = 3.0f;
              }
              if (ImGui::Button("4")) {
               renderer->state.vct.metallic_aperature = 4.0f;
              }
              if (ImGui::Button("5")) {
               renderer->state.vct.metallic_aperature = 5.0f;
              }
              ImGui::SliderFloat("Trace distance", &renderer->state.vct.specular_cone_trace_distance, 0.0625f, 1.0f);
              ImGui::SameLine();
              ImGui_HelpMarker("Specular cone trace distance in terms of factor of max scene length");
              if (ImGui::Button("1")) {
               renderer->state.vct.specular_cone_trace_distance = 1.0f; 
              }
              if (ImGui::Button("1/2")) {
               renderer->state.vct.specular_cone_trace_distance = 1.0f / 2.0f;
              }
              if (ImGui::Button("1/4")) {
               renderer->state.vct.specular_cone_trace_distance = 1.0f / 4.0f;
              }
              if (ImGui::Button("1/8")) {
               renderer->state.vct.specular_cone_trace_distance = 1.0f / 8.0f;
              }
              if (ImGui::Button("1/16")) {
               renderer->state.vct.specular_cone_trace_distance = 1.0f / 16.0f;
              }
              ImGui::TreePop();
            }
          }

          if (ImGui::CollapsingHeader("Bilateral filtering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.bilateral_filtering.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.bilateral_filtering.execution_time[0]) / float(renderer->state.total_execution_time));

            ImGui::Checkbox("Enabled##filtering", &renderer->state.bilateral_filtering.enabled);

            ImGui::Checkbox("Normalmapping##filtering", &renderer->state.bilateral_filtering.normalmapping);

            ImGui::Text("Filter:");
            ImGui::Checkbox("Direct##filtering", &renderer->state.bilateral_filtering.direct);
            ImGui::SameLine();
            ImGui::Checkbox("Indirect##filtering", &renderer->state.bilateral_filtering.indirect);

            ImGui::Checkbox("Specular##filtering", &renderer->state.bilateral_filtering.specular);
            ImGui::SameLine();
            ImGui::Checkbox("Ambient##filtering", &renderer->state.bilateral_filtering.ambient);

            ImGui::Separator();

            ImGui::Text("Gaussian spatial kernel weights");
            ImGui::BeginChild("spatial_kernel", ImVec2(0, 3.5f * ImGui::GetTextLineHeight()), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (const float& weight : renderer->state.bilateral_filtering.kernel) {
              ImGui::SameLine(); ImGui::Text("%0.2f", weight);
            }
            ImGui::EndChild();

            ImGui::InputInt("Spatial kernel radius", (int*) (&renderer->state.bilateral_filtering.spatial_kernel_radius));
            ImGui::InputFloat("Spatial kernel sigma", &renderer->state.bilateral_filtering.spatial_kernel_sigma);

            ImGui::Separator();

            ImGui::Text("Weights:");
            ImGui::Checkbox("Position", &renderer->state.bilateral_filtering.position_weight);
            ImGui::SameLine(); ImGui::SliderFloat("Sigma##Position", &renderer->state.bilateral_filtering.position_sigma, 0.05f, 8.0f);

            ImGui::Checkbox("Normal", &renderer->state.bilateral_filtering.normal_weight);
            ImGui::SameLine(); ImGui::SliderFloat("Sigma##Normal", &renderer->state.bilateral_filtering.normal_sigma, 0.05f, 8.0f);

            ImGui::Checkbox("Depth", &renderer->state.bilateral_filtering.depth_weight);
            ImGui::SameLine(); ImGui::SliderFloat("Sigma##Depth", &renderer->state.bilateral_filtering.depth_sigma, 0.05f, 8.0f);
          }

          if (ImGui::CollapsingHeader("Upsampling", ImGuiTreeNodeFlags_DefaultOpen)) {
            static int s = 0;
            ImGui::RadioButton("Joint bilateral", &s, 0); ImGui::SameLine();
            ImGui::RadioButton("Bilinear", &s, 1);
            ImGui::RadioButton("Nearest neighbor", &s, 2);

            ImGui::Separator();

            // Joint bilateral upsampling
            if (s == 0) {
              renderer->state.bilateral_upsample.enabled = true;
              renderer->state.bilinear_upsample.enabled = false;

              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.bilateral_upsample.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.bilateral_upsample.execution_time[0]) / float(renderer->state.total_execution_time));

              ImGui::Text("Radiance:");
              ImGui::Checkbox("Indirect##bilateralupsample", &renderer->state.bilateral_upsample.indirect);
              ImGui::SameLine();
              ImGui::Checkbox("Specular##bilateralupsample", &renderer->state.bilateral_upsample.specular);
              ImGui::SameLine();
              ImGui::Checkbox("Ambient##bilateralupsample", &renderer->state.bilateral_upsample.ambient);

              ImGui::Separator();

              ImGui::Text("Weights:");
              ImGui::Checkbox("Depth##bilateralupsample", &renderer->state.bilateral_upsample.depth_weight);
              ImGui::SameLine();
              ImGui::Checkbox("Normal##bilateralupsample", &renderer->state.bilateral_upsample.normal_weight);
              ImGui::SameLine();
              ImGui::Checkbox("Position##bilateralupsample", &renderer->state.bilateral_upsample.position_weight);

              ImGui::Checkbox("Normal mapping##bilateralupsample", &renderer->state.bilateral_upsample.normal_mapping);
            }

            // Bilinear upsampling
            if (s == 1 || s == 2) {
              renderer->state.bilateral_upsample.enabled = false;
              renderer->state.bilinear_upsample.enabled = true;

              // Nearest neighbor
              if (s == 2) {
                renderer->state.bilinear_upsample.nearest_neighbor = true;
              }

              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.bilinear_upsample.execution_time[0] / 1'000'000.0f, 100.0f * float(renderer->state.bilinear_upsample.execution_time[0]) / float(renderer->state.total_execution_time));

              ImGui::Checkbox("Indirect##bilinear", &renderer->state.bilinear_upsample.indirect);
              ImGui::SameLine();
              ImGui::Checkbox("Specular##bilinear", &renderer->state.bilinear_upsample.specular);
              ImGui::SameLine();
              ImGui::Checkbox("Ambient##bilinear", &renderer->state.bilinear_upsample.ambient);
            }
          }

          ImGui::PopItemWidth();
          ImGui::End();
        }

        if (Gui.scene_graph_window) {
          ImGui::Begin("Scene graph", &Gui.scene_graph_window, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize);

          if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("New##scene-graph")) {
              if (ImGui::MenuItem("Model")) {
                // TODO: Model loader GUI
              }
              if (ImGui::BeginMenu("Geometric primitive")) {
                if (ImGui::MenuItem("Cube")) {
                  world.spawn_entity(MeshPrimitive::Cube);
                }
                if (ImGui::MenuItem("Sphere")) {
                  world.spawn_entity(MeshPrimitive::Sphere);
                }
                ImGui::EndMenu();
              }
              ImGui::EndMenu();
              ImGui::Separator();
              if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Pointlight")) {
                  // TODO: Spawn pointlight
                }
                ImGui::EndMenu();
              }
            }
            ImGui::EndMenuBar();
          }

          if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat3("Position##camera", &renderer->scene->camera.position.x);
            ImGui::InputFloat3("Direction##camera", &renderer->scene->camera.direction.x);
            // ImGui::InputFloat("FoV", &renderer->scene->camera.FoV); // TODO: Camera adjustable FoV
            ImGui::SameLine();
            if (ImGui::Button("Reset")) { renderer->scene->reset_camera(); }
          }

          // Directional light
          if (ImGui::CollapsingHeader("Directional light")) {
            ImGui::InputFloat3("Direction##directional_light", &renderer->scene->directional_light.direction.x);
            ImGui::SameLine(); ImGui_HelpMarker("Direction is negative.");

            ImGui::ColorEdit3("Intensity##directional_light", &renderer->scene->directional_light.intensity.x);
            ImGui::SameLine(); ImGui_HelpMarker("Intensity or color of the light."); // FIXME: May or may not exceed 1.0??
          }

          // Point lights
          const std::string pointlights_title = "Point lights (" + std::to_string(renderer->pointlights.size()) + ")";
          if (ImGui::CollapsingHeader(pointlights_title.c_str())) {
            for (size_t i = 0; i < renderer->pointlights.size(); i++) {
              ImGui::PushID(&renderer->pointlights[i]);
              const std::string str = std::to_string(i);
              if (ImGui::CollapsingHeader(str.c_str())) {
                ImGui::InputFloat3("Position##pointlight", &renderer->pointlights[i].position.x);
                ImGui::ColorEdit3("Intensity##pointlight", &renderer->pointlights[i].intensity.x);
                ImGui::SameLine(); ImGui_HelpMarker("Intensity or color of the light."); // FIXME: May or may not exceed 1.0??
              }
              ImGui::PopID();
            }
          }

          // Graphics batches
          const std::string graphicsbatches_title = "Graphics batches (" + std::to_string(renderer->graphics_batches.size()) + ")";
          if (ImGui::CollapsingHeader(graphicsbatches_title.c_str())) {
            for (size_t batch_num = 0; batch_num < renderer->graphics_batches.size(); batch_num++) {
              auto& batch = renderer->graphics_batches[batch_num];
             
              const std::string batch_title = "Batch #" + std::to_string(batch_num + 1) + " (" + std::to_string(batch.entity_ids.size()) + ")";
              if (ImGui::CollapsingHeader(batch_title.c_str())) {

                if (ImGui::Button("Delete batch")) {
                  // TODO: Delete the whole batch and all the components
                }

                const std::string shader_title = "Shader##" + batch_title;
                if (ImGui::CollapsingHeader(shader_title.c_str())) {
                  // TODO: Open shader view
                  // ImGui::Text(batch.shader.vertex_filepath);
                  // ImGui::Checkbox("Vertex shader: [X]");
                }

                const std::string member_title = "Members##" + batch_title;
                if (ImGui::CollapsingHeader(member_title.c_str())) {
                  for (const auto& [id, idx]  : batch.data_idx) {
                    ImGui::Text("Entity id: %lu", id);

                    const std::string* name = NameSystem::instance().get_name_from_entity_referenced(id);
                    char new_name[128] = {0};
                    ImGui::InputText("Name", new_name, sizeof(new_name));
                    if (ImGui::Button("Confirm")) {
                      // NameSystem::instance().update_name(id, std::string(new_name));
                    }

                    TransformComponent* transform = TransformSystem::instance().lookup_referenced(id);
                    ImGui::PushID(transform);
                    ImGui::InputFloat3("Position", &transform->position.x);
                    ImGui::InputFloat3("Rotation", &transform->rotation.x);
                    ImGui::InputFloat("Scale", &transform->scale);

                    switch (batch.objects.materials[idx].shading_model) {
                    case ShadingModel::Unlit:
                      break;
                    case ShadingModel::PhysicallyBased:
                      // TODO: Show texture used for various properties
                      break;
                    case ShadingModel::PhysicallyBasedScalars:
                      float* emissive = &batch.gl_material_buffer_ptr[idx].emissive_scalars.x;
                      ImGui::ColorEdit3("Emissive", emissive);
                      float* diffuse = &batch.gl_material_buffer_ptr[idx].diffuse_scalars.x;
                      ImGui::ColorEdit3("Diffuse", diffuse);
                      break;
                    }

                    if (ImGui::Button("Remove")) {
                      renderer->remove_component(id);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clone")) {
                      NameSystem::instance().add_name_to_entity(std::string(new_name), id);
                    }

                    ImGui::PopID();
                  }
                }
              }
            }
          }

          ImGui::End();
        }

        if (Gui.logger_window) {
          LoggingSystem::instance().draw_gui(&Gui.logger_window);
        }

        if (Gui.network_window) {
          // NetworkSystem::instance().draw_gui(&Gui.network_window);
        }

        if (Gui.scripting_window) {
          ImGui::SetWindowSize(ImVec2(500, 450), ImGuiSetCond_Once);

          if (ImGui::Begin("Scripting window")) {
            keyboard_enabled = !ImGui::IsRootWindowOrAnyChildFocused();

            // static char buf[1024] = {"start:\n"
            //                       "\tADDI 0 1\n"
            //                       "\tCALL Log::info\n"
            //                       "\tJUMP start\n"};
            static char buf[1024] = {"start:\n"
                                  "\tADDI 0 1\n"
                                  "\tCALL Log::info\n"
                                  "\tCMPI 0 4\n"
                                  "\tBRNEQ start\n"
                                  "\tCALL exit\n"};
            // static char buf[1024] = {"start:\n"
            //                       "\tCALL counter\n"
            //                       "\tBRNEQ start\n"
            //                       "\tCALL exit\n"
            //                      "counter:\n"
            //                       "\tADDI 0 1\n"
            //                       "\tCALL Log::info\n"
            //                       "\tCMPI 0 4\n"
            //                       "\tRET"
            // };

            if (ImGui::Button("Run")) {
              MkAssContext& ctx = MkAssProgramManager::instance().new_ctx();
              ctx.register_external_symbol("log::info", [](MkAssContext& ctx){
                                                          Log::info("\t[MkAssContext]: " + std::to_string(ctx.regs[0]));
                                                        });
              ctx.register_external_symbol("exit",      [](MkAssContext& ctx){
                                                          Log::info("\t[MkAssContext]: exited");
                                                          ctx.exit = true;
                                                        });
              MkAssProgramManager::instance().run(ctx, std::string(buf));
            }

            ImGui::BeginChild("##script", ImVec2(200,200), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::InputTextMultiline(" ", buf, sizeof(buf));
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("##runtime", ImVec2(200,200), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::InputTextMultiline(" ", buf, sizeof(buf));
            ImGui::EndChild();

            MkAssProgramManager::instance().draw_gui();


            ImGui::End();
          }
        }

        if (Gui.console_window) {
          if (ImGui::Begin("Console window")) {
            keyboard_enabled = !ImGui::IsRootWindowOrAnyChildFocused();
            ImGui::Text("> ls -a -systems");
            ImGui::End();
          }
        }

        ImGui::Render();
        end_gl_cmds();
      }
    }
    SDL_GL_SwapWindow(window);
  }
  // TODO: Config::save_scene
  // Config::save_scene(renderer->scene);
}
