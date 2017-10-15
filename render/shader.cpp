#include "shader.h"
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <SDL_log.h>

static void log_gl_error() {
  GLenum err = glGetError();
  switch(err) {
    case GL_INVALID_VALUE:
      SDL_Log("GL_INVALID_VALUE");
      break;
    default:
      if (err != 0) {
        std::cout << glewGetErrorString(err) << std::endl;
        SDL_Log("OpenGL error: %i", err);
      }
      break;
  }
}

Shader::Shader(std::string vertex_filepath, std::string fragment_filepath): vertex_filepath(vertex_filepath),
                                                                          fragment_filepath(fragment_filepath),
                                                                          vertex_shader(0),
                                                                          fragment_shader(0),
                                                                          gl_program(0),
                                                                          defines{} {}

std::pair<bool, std::string> Shader::compile() {
  if (!file_exists(vertex_filepath) || !file_exists(fragment_filepath)) {
      return {false, "File(s) do not exists: " + vertex_filepath + " and/or " + fragment_filepath};
  }

  auto vertex_src   = load_shader_source(vertex_filepath);
  auto fragment_src = load_shader_source(fragment_filepath);

  for (auto &define : defines) {
      vertex_src.insert(0, define);
      fragment_src.insert(0, define);
  }

  vertex_src.insert(0, "#version 410 core \n");
  fragment_src.insert(0, "#version 410 core \n");

  auto raw_str = vertex_src.c_str();
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &raw_str, NULL);

  raw_str = fragment_src.c_str();
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &raw_str, NULL);
  
  GLint shader_program = glCreateProgram();
  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  GLint vertex_shader_status;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_status);

  GLint fragment_shader_status;
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_shader_status);

  // FIXME: Always detach shaders after a successful link.
  // glDetachShader(shader_program, vertex_shader);
  // glDetachShader(shader_program, fragment_shader);
  // glDeleteShader(vertex_shader);
  // glDeleteShader(fragment_shader);
  
  if (vertex_shader_status == GL_TRUE && fragment_shader_status == GL_TRUE) {
      gl_program = shader_program;
      return {true, ""};
  }

  GLint err_size = 0;
  glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &err_size);
  char vert_err_msg[err_size];
  glGetShaderInfoLog(vertex_shader, err_size, NULL, &vert_err_msg[0]);

  glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &err_size);
  char frag_err_msg[err_size];
  glGetShaderInfoLog(fragment_shader, err_size, NULL, &frag_err_msg[0]);

  glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &err_size);
  char prog_err_msg[err_size];
  glGetProgramInfoLog(shader_program, err_size, NULL, &prog_err_msg[0]);

  auto gl_error = glGetError();
  SDL_Log("OpenGL Error: 0x%08x", gl_error);
  log_gl_error();
  
  glDeleteProgram(shader_program);

  return {false, std::string(vert_err_msg) + std::string(frag_err_msg) + std::string(prog_err_msg)};
};

std::pair<bool, std::string> Shader::recompile() {
  auto vertex_src   = load_shader_source(vertex_filepath);
  auto fragment_src = load_shader_source(fragment_filepath);

  glDetachShader(gl_program, vertex_shader);
  glDetachShader(gl_program, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  vertex_shader   = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  auto raw_str = vertex_src.c_str();
  glShaderSource(vertex_shader, 1, &raw_str, NULL);

  raw_str = fragment_src.c_str();
  glShaderSource(fragment_shader, 1, &raw_str, NULL);

  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);

  GLint vertex_shader_status;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_status);

  GLint fragment_shader_status;
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_shader_status);

  if (vertex_shader_status == GL_TRUE && fragment_shader_status == GL_TRUE) {
      // Relink shader program
      glAttachShader(gl_program, vertex_shader);
      glAttachShader(gl_program, fragment_shader);
      glLinkProgram(gl_program);
      GLint is_linked = GL_FALSE;
      glGetProgramiv(gl_program, GL_LINK_STATUS, &is_linked);
      SDL_Log("Shader relinking is success: %i", is_linked == GL_TRUE);

      if (is_linked) {
          // Always detach shaders after a successful link.
          glDetachShader(gl_program, vertex_shader);
          glDetachShader(gl_program, fragment_shader);
          glDeleteShader(vertex_shader);
          glDeleteShader(fragment_shader);
      }

      std::string err_log = "";
      GLint max_log_lng = 0;
      glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &max_log_lng);
      err_log.reserve(max_log_lng);
      glGetProgramInfoLog(gl_program, max_log_lng, NULL, (char *) err_log.c_str());
      SDL_Log("%s", err_log.c_str());

      return {true, ""};
  }

  uint32_t err_size = 512;
  std::string frag_err_msg;
  std::string vert_err_msg;
  frag_err_msg.reserve(err_size);
  vert_err_msg.reserve(err_size);
  glGetShaderInfoLog(fragment_shader, err_size, NULL, (char *) frag_err_msg.c_str());
  glGetShaderInfoLog(vertex_shader  , err_size, NULL, (char *) vert_err_msg.c_str());
  return {false, frag_err_msg + vert_err_msg};
};

bool Shader::file_exists(std::string filename) const {
  std::ifstream ifs(filename);
  return ifs.good();
}

const std::string Shader::load_shader_source(std::string filename) const {
  std::ifstream ifs(filename);
  return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

void Shader::add(std::string define) {
  defines.insert(define);
}

uint32_t Shader::get_uniform_location(Texture::Type type) {
  std::string uniform_location;
  switch (type) {
    case Texture::Type::Diffuse:
      uniform_location = "diffuse_sampler";
    default:
      break;
  }
  glUseProgram(gl_program);
  auto res = glGetUniformLocation(gl_program, uniform_location.c_str());
  if (res < 0) { SDL_Log("glGetUniformLocation failed"); log_gl_error(); return 0; }
  return res;
}

bool Shader::operator==(const Shader &rhs) {
 return defines == rhs.defines;
}

bool Shader::validate() {
  // Might need to model the dependencies of the defines in a graph at compile time which speeds the validation along
  // TODO: This will check if the defines plays nicely together or not. No point in compiling a faulty shader.
  return true;
}

