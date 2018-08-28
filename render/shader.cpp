#include "shader.h"

#include <iostream>
#include <fstream>
#include <SDL_log.h>

#include "debug_opengl.h"

#ifdef _WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif 

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

#ifdef DEBUG
  vertex_shader_src = vertex_src;
  fragment_shader_src = fragment_src;
#endif 

  auto raw_str = vertex_src.c_str();
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &raw_str, nullptr);

  raw_str = fragment_src.c_str();
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &raw_str, nullptr);
  
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

  glDetachShader(shader_program, vertex_shader);
  glDetachShader(shader_program, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  if (vertex_shader_status == GL_TRUE && fragment_shader_status == GL_TRUE) {
      gl_program = shader_program;
      return {true, ""};
  }

  GLint err_size = 1024;
  glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &err_size);
  char* vert_err_msg = new char[1024];
  glGetShaderInfoLog(vertex_shader, err_size, nullptr, vert_err_msg);
  SDL_Log("%s", vert_err_msg);

  glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &err_size);
  char* frag_err_msg = new char[1024];
  glGetShaderInfoLog(fragment_shader, err_size, nullptr, frag_err_msg);
  SDL_Log("%s", frag_err_msg);
  
  glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &err_size);
  char* prog_err_msg = new char[1024];
  glGetProgramInfoLog(shader_program, err_size, nullptr, prog_err_msg);
  SDL_Log("%s", prog_err_msg);
  
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
  glShaderSource(vertex_shader, 1, &raw_str, nullptr);

  raw_str = fragment_src.c_str();
  glShaderSource(fragment_shader, 1, &raw_str, nullptr);

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
      glGetProgramInfoLog(gl_program, max_log_lng, nullptr, (char *) err_log.c_str());
      SDL_Log("%s", err_log.c_str());

      return {true, ""};
  }

  uint32_t err_size = 512;
  std::string frag_err_msg;
  std::string vert_err_msg;
  frag_err_msg.reserve(err_size);
  vert_err_msg.reserve(err_size);
  glGetShaderInfoLog(fragment_shader, err_size, nullptr, (char *) frag_err_msg.c_str());
  glGetShaderInfoLog(vertex_shader  , err_size, nullptr, (char *) vert_err_msg.c_str());
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

bool Shader::operator==(const Shader &rhs) {
 return defines == rhs.defines;
}

bool Shader::validate() {
  // Might need to model the dependencies of the defines in a graph at compile time which speeds the validation along
  // TODO: This will check if the defines plays nicely together or not. No point in compiling a faulty shader.
  return true;
}

