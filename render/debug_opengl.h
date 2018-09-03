#ifndef MEINEKRAFT_DEBUG_OPENGL_H
#define MEINEKRAFT_DEBUG_OPENGL_H

#ifdef WIN32 
#include <glew.h>
#include <SDL_opengl.h>
#else
#include <glew.h>
#include <SDL_opengl.h>
#endif

static void GLAPIENTRY gl_debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* user_param)
{
  std::cerr << " ----- GL ERROR CALLBACK ----- " << std::endl;
  
  std::cerr << "Type: ";
  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    std::cerr << "GL ERROR";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    std::cerr << "DEPRECATED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    std::cerr << "UNDEFINED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    std::cerr << "PORTABILITY";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    std::cerr << "PERFORMANCE";
    break;
  case GL_DEBUG_TYPE_OTHER:
    std::cerr << "OTHER";
    break;
  default:
    std::cerr << "?";
  }
  std::cerr << std::endl;

  std::cerr << "Severity: ";
  switch (severity) {
  case GL_DEBUG_SEVERITY_LOW:
    std::cerr << "LOW";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    std::cerr << "MEDIUM";
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    std::cerr << "HIGH";
    break;
  default: 
    std::cerr << "?";
  }
  std::cerr << std::endl;

  std::cerr << "Type: " << glewGetErrorString(type) << std::endl;
  std::cerr << "Message: " << message << std::endl;
  std::cerr << " ----- ----- ----- ----- ----- " << std::endl;
  std::cerr << std::endl;
}

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
