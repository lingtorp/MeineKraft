#ifndef MEINEKRAFT_DEBUG_OPENGL_H
#define MEINEKRAFT_DEBUG_OPENGL_H

#ifdef WIN32 
#include <glew.h>
#include <SDL_opengl.h>
#endif

static void log_gl_error() {
  GLenum err = glGetError();
  std::string err_str;
  switch(err) {
    case GL_NO_ERROR:
      return;
    case GL_INVALID_VALUE:
      err_str = "GL_INVALID_VALUE";
      break;
    case GL_INVALID_ENUM:
      err_str = "GL_INVALID_ENUM";
      break;
    case GL_INVALID_OPERATION:
      err_str = "GL_INVALID_OPERATION";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      err_str = "GL_INVALID_FRAMEBUFFER_OPERTION";
      break;
    case GL_OUT_OF_MEMORY:
      err_str = "GL_OUT_OF_MEMORY";
      break;
    case GL_STACK_OVERFLOW:
      err_str = "GL_STACK_OVERFLOW";
      break;
    case GL_STACK_UNDERFLOW:
      err_str = "GL_STACK_UNDERFLOW";
      break;
    default:
      err_str = "UNKNOWN ERROR";
      break;
  }
  std::cerr << glewGetErrorString(err) << std::endl;
  SDL_Log("OpenGL error (%s): 0x%X (%i)", err_str.c_str(), err, err);
}

#endif // MEINEKRAFT_DEBUG_OPENGL_H
