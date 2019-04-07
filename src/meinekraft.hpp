#pragma once 
#ifndef MEINEKRAFT_APPLICATION_HPP
#define MEINEKRAFT_APPLICATION_HPP

/// NOTE: This header is forbidden from including other headers (except those already included here)

#if defined(WIN32)
#define OPENGL_MINOR_VERSION 6 // Windows 10
#elif defined(__linux__)
#define OPENGL_MINOR_VERSION 5 // Ubuntu 18.10 (default)
#elif defined(__APPLE__)
#define OPENGL_MINOR_VERSION 1 // macOS (deprecated)
#endif

struct Renderer;
struct SDL_Window;

struct MeineKraft {
    MeineKraft();
    ~MeineKraft();

    static MeineKraft& instance() {
      static MeineKraft mk;
      return mk;
    };

    Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;

    /// MeineKraft initialization is done when this is called
    void init();

    /// MeineKraft engine mainloop
    void mainloop();
};

#endif // MEINEKRAFT_APPLICATION_HPP