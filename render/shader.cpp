#include "shader.hpp"

#include <fstream>

#include "debug_opengl.hpp"

#ifdef _WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif 

#if defined(__APPLE__)
static const char* GLSL_VERSION = "#version 410 core \n";
#elif defined(__linux__)
static const char* GLSL_VERSION = "#version 450 core \n";
#elif defined(WIN32)
static const char* GLSL_VERSION = "#version 460 core \n";
#endif

/// Check whether a file exists or not
bool file_exists(std::string filename) {
  std::ifstream ifs(filename);
  return ifs.good();
}

/// Reads the entire contents of the file and returns it
const std::string load_shader_source(const std::string& filename) {
  std::ifstream ifs(filename);
  return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

ComputeShader::ComputeShader(const std::string& compute_filepath) {
  GLuint gl_comp_shader = glCreateShader(GL_COMPUTE_SHADER);
  std::string comp_src = load_shader_source(compute_filepath);

  // Insert the define into the shader src
  comp_src.insert(0, GLSL_VERSION);

  const char* raw_str_ptr = comp_src.c_str();
  glShaderSource(gl_comp_shader, 1, &raw_str_ptr, nullptr);
  glCompileShader(gl_comp_shader);

  gl_program = glCreateProgram();
  glAttachShader(gl_program, gl_comp_shader);
  glLinkProgram(gl_program);

  GLint compute_shader_status = 0;
  glGetShaderiv(gl_comp_shader, GL_COMPILE_STATUS, &compute_shader_status);

  glDetachShader(gl_program, gl_comp_shader);
  glDeleteShader(gl_comp_shader);

  if (!compute_shader_status) { std::cerr << "Could not compile compute shader, aborting ... " << std::endl; exit(-1); }
}

Shader::Shader(const std::string& vertex_filepath, const std::string& fragment_filepath):
  vertex_filepath(vertex_filepath), fragment_filepath(fragment_filepath),
  gl_vertex_shader(0), gl_fragment_shader(0), gl_program(0), defines{} {}

std::pair<bool, std::string> Shader::compile() {
  if (!file_exists(vertex_filepath) || !file_exists(fragment_filepath)) {
      return {false, "File(s) do not exists: " + vertex_filepath + " and/or " + fragment_filepath};
  }

  vertex_src   = load_shader_source(vertex_filepath);
  fragment_src = load_shader_source(fragment_filepath);

  for (const auto& define : defines) {
      vertex_src.insert(0, Shader::shader_define_to_string(define));
      fragment_src.insert(0, Shader::shader_define_to_string(define));
  }

  vertex_src.insert(0, GLSL_VERSION);
  fragment_src.insert(0, GLSL_VERSION);

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
