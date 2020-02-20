#pragma once 
#ifndef MEINEKRAFT_APPLICATION_HPP
#define MEINEKRAFT_APPLICATION_HPP

/// NOTE: This header is forbidden from including other headers
#include <stdint.h>

#if defined(WIN32)
#define OPENGL_MINOR_VERSION 6 // Windows 10
#elif defined(__linux__)
#define OPENGL_MINOR_VERSION 6 // Ubuntu 18.10 (default)
#elif defined(__APPLE__)
#define OPENGL_MINOR_VERSION 1 // macOS (deprecated)
#endif

struct Renderer;
struct SDL_Window;

/// Main struct of the engine
struct MeineKraft {
    MeineKraft();
    ~MeineKraft();

    static MeineKraft& instance() {
      static MeineKraft mk;
      return mk;
    };

    /// Screenshot mode lets the engine run for a couple of frames and then takes a screenshot and quits
    bool screenshot_mode = false;
    /// Number of frames to render before taking screenshot
    const uint8_t screenshot_mode_frame_wait = 25;

    SDL_Window* window = nullptr;
    Renderer* renderer = nullptr;

    /// MeineKraft initialization is done when this is called
    void init();

    /// MeineKraft engine mainloop
    void mainloop();
};

#endif // MEINEKRAFT_APPLICATION_HPP
