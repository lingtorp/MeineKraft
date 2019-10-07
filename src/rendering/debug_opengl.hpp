#pragma once
#ifndef MEINEKRAFT_DEBUG_OPENGL_HPP
#define MEINEKRAFT_DEBUG_OPENGL_HPP

#include <glew.h>
#include <SDL_opengl.h> 

#include "../util/logging.hpp"

#include <sstream>

/// Gathers information about the OpenGL context 
struct OpenGLContextInfo {
  // TODO: Document each of the member variables??
  int max_texture_units;
  int max_fbo_color_attachments;
  int max_fbo_attachment_width;
  int max_fbo_attachment_height;
  int max_fbo_layers;
  int max_draw_buffers;
  int max_texture_array_layers;
  int max_image_texture_units;

  bool GL_NV_shader_atomic_float_supported = false;

  OpenGLContextInfo(const size_t gl_major_version,
                    const size_t gl_minor_version) {
    Log::info(glGetString(GL_VERSION) == nullptr ? "null" : "smt");
    Log::info("OpenGL version: " + std::string((const char*)glGetString(GL_VERSION)));
    Log::info("OpenGL version: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
    Log::info("GLSL: " + std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))));
    Log::info("Vendor: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR))));
    Log::info("Renderer: " + std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));

    // #define VERBOSE
    #ifdef VERBOSE
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    const char **extensions = (const char **)malloc(n * sizeof(char *));
    if (n > 0) {
      for (GLint i = 0; i < n; i++) {
        extensions[i] = (char*)glGetStringi(GL_EXTENSIONS, i);
        Log::info(std::string(extensions[i]));
      }
    }
    delete extensions;
    #endif

    // Handle extensions
    if (glewIsExtensionSupported("GL_NV_shader_atomic_float")) {
      GL_NV_shader_atomic_float_supported = true;
    }

    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
    Log::info("Max draw buffers: " + std::to_string(max_draw_buffers));

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_fbo_color_attachments);
    Log::info("Max FBO color attachments: " + std::to_string(max_fbo_color_attachments));

    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &max_fbo_attachment_width);
    Log::info("Max FBO attachment width and height: " + std::to_string(max_fbo_attachment_width) + " / " + std::to_string(max_fbo_attachment_height));

    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &max_fbo_layers);
    Log::info("Max FBO layers: " + std::to_string(max_fbo_layers));

    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_texture_array_layers);
    Log::info("Max texture array layers/elements: " + std::to_string(max_texture_array_layers));

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
    Log::info("Max texture units: " + std::to_string(max_texture_units));

    glGetIntegerv(GL_MAX_IMAGE_UNITS, &max_image_texture_units);
    Log::info("Max image texture units: " + std::to_string(max_image_texture_units));
  }
};

// Prints debug messages - notifications
// #define DEBUG_NOTIFICATIONS
// Prints debug messages - performance
// #define DEBUG_PERFORMANCE

static void GLAPIENTRY gl_debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* user_param)
{
  std::stringstream ss;
  ss << " ----- GL ERROR CALLBACK ----- " << std::endl;
  
  ss << "Type: ";
  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    ss << "GL ERROR";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    ss << "DEPRECATED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    ss << "UNDEFINED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    ss << "PORTABILITY";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    ss << "PERFORMANCE";
#ifndef DEBUG_PERFORMANCE
    return;
#endif
    break;
  case GL_DEBUG_TYPE_OTHER:
    ss << "OTHER";
    break;
  default:
    ss << "?";
  }
  ss << std::endl;

  ss << "Severity: ";
  switch (severity) {
  case GL_DEBUG_SEVERITY_LOW:
    ss << "LOW";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    ss << "MEDIUM";
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    ss << "HIGH";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
	  ss << "NOTIFICATION";
#ifndef DEBUG_NOTIFICATION
    return;
#endif
	  break;
  default: 
    ss << "?";
  }
  ss << std::endl;

  ss << "Type: " << glewGetErrorString(type) << std::endl;
  ss << "Message: " << message << std::endl;
  ss << " ----- ----- ----- ----- ----- " << std::endl;  
  ss << std::endl;
  Log::error(ss.str());
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
