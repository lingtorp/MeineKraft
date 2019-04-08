#pragma once
#ifndef MEINEKRAFT_DEBUG_OPENGL_HPP
#define MEINEKRAFT_DEBUG_OPENGL_HPP

#include <glew.h>
#include <SDL_opengl.h> 

/// Gathers information about the OpenGL context 
struct OpenGLContextInfo {
  // TODO: Document each of the member variables??
  int max_texture_units;
  int max_color_attachments;
  int max_draw_buffers;
  int max_texture_array_layers;

  OpenGLContextInfo(const size_t gl_major_version, const size_t gl_minor_version) {
    Log::info("OpenGL version: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
    Log::info("GLSL: " + std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))));
    Log::info("Vendor: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR))));
    Log::info("Renderer: " + std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));

    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
    Log::info("Max draw buffers: " + std::to_string(max_draw_buffers));

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
    Log::info("Max color attachments: " + std::to_string(max_color_attachments));

    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_texture_array_layers);
    Log::info("Max texture array layers/elements: " + std::to_string(max_texture_array_layers));
  }
};

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
  case GL_DEBUG_SEVERITY_NOTIFICATION:
	  std::cerr << "NOTIFICATION";
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
  Log::error(glewGetErrorString(err));
  Log::error("OpenGL error " + err_str + ":" + std::to_string(err));
}

#endif // MEINEKRAFT_DEBUG_OPENGL_HPP
