#include "shader.h"

#include <fstream>

#include "debug_opengl.h"

#ifdef _WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif 

Shader::Shader(const std::string& vertex_filepath, const std::string& fragment_filepath):
  vertex_filepath(vertex_filepath), fragment_filepath(fragment_filepath),
  gl_vertex_shader(0), gl_fragment_shader(0), gl_program(0), defines{} {}

std::pair<bool, std::string> Shader::compile() {
  if (!file_exists(vertex_filepath) || !file_exists(fragment_filepath)) {
      return {false, "File(s) do not exists: " + vertex_filepath + " and/or " + fragment_filepath};
  }

  auto vertex_src   = load_shader_source(vertex_filepath);
  auto fragment_src = load_shader_source(fragment_filepath);

  for (auto &define : defines) {
      vertex_src.insert(0, Shader::shader_define_to_string(define));
      fragment_src.insert(0, Shader::shader_define_to_string(define));
  }

  static const char* GLSL_VERSION = "#version 460 core \n";
  vertex_src.insert(0, GLSL_VERSION);
  fragment_src.insert(0, GLSL_VERSION);

  vertex_shader_src = vertex_src;
  fragment_shader_src = fragment_src;

  auto raw_str = vertex_src.c_str();
  gl_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(gl_vertex_shader, 1, &raw_str, nullptr);

  raw_str = fragment_src.c_str();
  gl_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(gl_fragment_shader, 1, &raw_str, nullptr);
  
  GLint gl_shader_program = glCreateProgram();
  glCompileShader(gl_vertex_shader);
  glCompileShader(gl_fragment_shader);
  glAttachShader(gl_shader_program, gl_vertex_shader);
  glAttachShader(gl_shader_program, gl_fragment_shader);
  glLinkProgram(gl_shader_program);

  GLint vertex_shader_status = 0;
  glGetShaderiv(gl_vertex_shader, GL_COMPILE_STATUS, &vertex_shader_status);

  GLint fragment_shader_status = 0;
  glGetShaderiv(gl_fragment_shader, GL_COMPILE_STATUS, &fragment_shader_status);

  glDetachShader(gl_shader_program, gl_vertex_shader);
  glDetachShader(gl_shader_program, gl_fragment_shader);
  glDeleteShader(gl_vertex_shader);
  glDeleteShader(gl_fragment_shader);

  if (vertex_shader_status == GL_TRUE && fragment_shader_status == GL_TRUE) {
      gl_program = gl_shader_program;
      return {true, ""};
  }

  GLint err_size = 1024;
  glGetShaderiv(gl_vertex_shader, GL_INFO_LOG_LENGTH, &err_size);
  char* vert_err_msg = new char[1024];
  glGetShaderInfoLog(gl_vertex_shader, err_size, nullptr, vert_err_msg);
  Log::info(vert_err_msg);

  glGetShaderiv(gl_fragment_shader, GL_INFO_LOG_LENGTH, &err_size);
  char* frag_err_msg = new char[1024];
  glGetShaderInfoLog(gl_fragment_shader, err_size, nullptr, frag_err_msg);
  Log::info(frag_err_msg);
  
  glGetProgramiv(gl_shader_program, GL_INFO_LOG_LENGTH, &err_size);
  char* prog_err_msg = new char[1024];
  glGetProgramInfoLog(gl_shader_program, err_size, nullptr, prog_err_msg);
  Log::info(prog_err_msg);
  
  log_gl_error();
  
  glDeleteProgram(gl_shader_program);

  return {false, std::string(vert_err_msg) + std::string(frag_err_msg) + std::string(prog_err_msg)};
};

std::pair<bool, std::string> Shader::recompile() {
  auto vertex_src   = load_shader_source(vertex_filepath);
  auto fragment_src = load_shader_source(fragment_filepath);

  glDetachShader(gl_program, gl_vertex_shader);
  glDetachShader(gl_program, gl_fragment_shader);
  glDeleteShader(gl_vertex_shader);
  glDeleteShader(gl_fragment_shader);

  gl_vertex_shader   = glCreateShader(GL_VERTEX_SHADER);
  gl_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  auto raw_str = vertex_src.c_str();
  glShaderSource(gl_vertex_shader, 1, &raw_str, nullptr);

  raw_str = fragment_src.c_str();
  glShaderSource(gl_fragment_shader, 1, &raw_str, nullptr);

  glCompileShader(gl_vertex_shader);
  glCompileShader(gl_fragment_shader);

  GLint vertex_shader_status;
  glGetShaderiv(gl_vertex_shader, GL_COMPILE_STATUS, &vertex_shader_status);

  GLint fragment_shader_status;
  glGetShaderiv(gl_fragment_shader, GL_COMPILE_STATUS, &fragment_shader_status);

  if (vertex_shader_status == GL_TRUE && fragment_shader_status == GL_TRUE) {
      // Relink shader program
      glAttachShader(gl_program, gl_vertex_shader);
      glAttachShader(gl_program, gl_fragment_shader);
      glLinkProgram(gl_program);
      GLint is_linked = GL_FALSE;
      glGetProgramiv(gl_program, GL_LINK_STATUS, &is_linked);
      Log::info("Shader relinking is success: " + std::to_string(is_linked == GL_TRUE));

      if (is_linked) {
          // Always detach shaders after a successful link.
          glDetachShader(gl_program, gl_vertex_shader);
          glDetachShader(gl_program, gl_fragment_shader);
          glDeleteShader(gl_vertex_shader);
          glDeleteShader(gl_fragment_shader);
      }

      std::string err_log = "";
      GLint max_log_lng = 0;
      glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &max_log_lng);
      err_log.reserve(max_log_lng);
      glGetProgramInfoLog(gl_program, max_log_lng, nullptr, (char *) err_log.c_str());
      Log::info(err_log);

      return {true, ""};
  }

  uint32_t err_size = 512;
  std::string frag_err_msg;
  std::string vert_err_msg;
  frag_err_msg.reserve(err_size);
  vert_err_msg.reserve(err_size);
  glGetShaderInfoLog(gl_fragment_shader, err_size, nullptr, (char *) frag_err_msg.c_str());
  glGetShaderInfoLog(gl_vertex_shader  , err_size, nullptr, (char *) vert_err_msg.c_str());
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

std::string Shader::shader_define_to_string(const Shader::Defines define) {
  switch (define) {
  case Shader::Defines::Diffuse2D:
    return "#define DIFFUSE_2D \n";
  case Shader::Defines::DiffuseCubemap:
    return "#define DIFFUSE_CUBEMAP \n";
  default:
    Log::error("Invalid shader define passed");
    return "";
  }
}

void Shader::add(const Shader::Defines define) {
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
