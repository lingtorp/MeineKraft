#pragma once 
#ifndef MEINEKRAFT_APPLICATION_HPP
#define MEINEKRAFT_APPLICATION_HPP

/// NOTE: This header is forbidden from including other headers (except those already included here)

struct Resolution {
  int width, height;
};

static auto HD      = Resolution{1280, 720};
static auto FULL_HD = Resolution{1920, 1080};

#if defined(WIN32)
#define OPENGL_MINOR_VERSION 6 // Windows 10
#elif defined(__linux__)
#define OPENGL_MINOR_VERSION 5 // Ubuntu 18.10 (default)
#elif defined(__APPLE__)
#define OPENGL_MINOR_VERSION 1 // macOS (deprecated)
#endif

struct Renderer;

struct MeineKraft {
    MeineKraft(MeineKraft& mk) = delete;
    MeineKraft(const MeineKraft& mk) = delete;
    MeineKraft();
    
    Renderer* renderer;

    static MeineKraft& instance() {
        static MeineKraft mk;
        return mk;
    }

    /// MeineKraft engine mainloop
    void mainloop();

private:
    MeineKraft();
};

#endif // MEINEKRAFT_APPLICATION_HPP