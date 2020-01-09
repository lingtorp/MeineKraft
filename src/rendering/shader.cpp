#include "shader.hpp"

#include "../util/filesystem.hpp"
#include "debug_opengl.hpp"

#include <string>

#ifdef _WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

#if defined(__APPLE__)
static const char *GLSL_VERSION = "#version 410 core \n";
#elif defined(__linux__)
static const char *GLSL_VERSION = "#version 450 core \n";
#elif defined(WIN32)
static const char *GLSL_VERSION = "#version 460 core \n";
#endif

static std::string shader_define_to_string(const Shader::Defines define) {
  switch (define) {
  case Shader::Defines::Diffuse2D:
    return "#define DIFFUSE_2D \n";
  case Shader::Defines::DiffuseCubemap:
    return "#define DIFFUSE_CUBEMAP \n";
  case Shader::Defines::DiffuseRGB:
    return "#define DIFFUSE_RGB \n";
    break;
  case Shader::Defines::DiffuseRGBA:
    return "#define DIFFUSE_RGBA \n";
    break;
  case Shader::Defines::Emissive:
    return "#define HAS_EMISSIVE_TEXTURE \n";
    break;
  default:
    Log::error("Invalid shader define passed");
    return "This will ensure that shader compilation does not work :)";
  }
}

static std::string shader_compilation_err_msg(const GLuint shader) {
  GLint max_lng = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_lng);

  if (max_lng == 0) {
    return "";
  }

  // The max_lng includes the NULL character
  char *err_log[max_lng];
  glGetShaderInfoLog(shader, max_lng, &max_lng, err_log[0]);

  return std::string(err_log[0]);
}

ComputeShader::ComputeShader(const std::string &compute_filepath,
                             const std::vector<std::string> &defines) {
  GLuint gl_comp_shader = glCreateShader(GL_COMPUTE_SHADER);
  std::string comp_src = Filesystem::read_file(compute_filepath);

  if (comp_src.empty()) {
    Log::error("Tried to compile empty compute shader ..");
    exit(-1);
  }

  for (const std::string &define : defines) {
    comp_src.insert(0, define);
  }

  // Insert the define into the shader src
  comp_src.insert(0, GLSL_VERSION);

  const char *raw_str_ptr = comp_src.c_str();
  glShaderSource(gl_comp_shader, 1, &raw_str_ptr, nullptr);
  glCompileShader(gl_comp_shader);

  glObjectLabel(GL_SHADER, gl_comp_shader, -1, compute_filepath.c_str());

  gl_program = glCreateProgram();
  glAttachShader(gl_program, gl_comp_shader);
  glLinkProgram(gl_program);

  glObjectLabel(GL_PROGRAM, gl_program, -1, compute_filepath.c_str());

  GLint compute_shader_status = 0;
  glGetShaderiv(gl_comp_shader, GL_COMPILE_STATUS, &compute_shader_status);

  glDetachShader(gl_program, gl_comp_shader);
  glDeleteShader(gl_comp_shader);

  if (compute_shader_status == GL_FALSE) {
    GLint err_size = 0;
    glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &err_size);
    char *prog_err_msg = new char[err_size];
    glGetProgramInfoLog(gl_program, err_size, nullptr, prog_err_msg);
    if (err_size != 0) {
      Log::error("Could not compile compute shader: " + compute_filepath +
                 "\n" + std::string(prog_err_msg));
    }
    delete[] prog_err_msg;

    exit(-1);
  }
}

Shader::Shader(const std::string &vertex_filepath,
               const std::string &geometry_filepath,
               const std::string &fragment_filepath)
    : vertex_filepath(vertex_filepath), geometry_filepath(geometry_filepath),
      fragment_filepath(fragment_filepath),
      vertex_src(Filesystem::read_file(vertex_filepath)),
      geometry_src(Filesystem::read_file(geometry_filepath)),
      fragment_src(Filesystem::read_file(fragment_filepath)) {}

Shader::Shader(const std::string &vertex_filepath,
               const std::string &fragment_filepath)
    : vertex_filepath(vertex_filepath), fragment_filepath(fragment_filepath),
      vertex_src(Filesystem::read_file(vertex_filepath)),
      fragment_src(Filesystem::read_file(fragment_filepath)) {}

std::pair<bool, std::string> Shader::compile() {
  const bool vertex_included = !vertex_filepath.empty();
  const bool geometry_included = !geometry_filepath.empty();
  const bool fragment_included = !fragment_filepath.empty();

  gl_program = glCreateProgram();

  GLint vertex_shader_compiled = GL_FALSE;
  GLint geometry_shader_compiled = GL_FALSE;
  GLint fragment_shader_compiled = GL_FALSE;

  if (!vertex_filepath.empty()) {
    if (vertex_src.empty()) {
      return {false, "Vertex shader passed could not be opened or is empty"};
    }

    for (const auto &define : defines) {
      vertex_src.insert(0, shader_define_to_string(define));
    }

    vertex_src.insert(0, GLSL_VERSION);

    const auto raw_str = vertex_src.c_str();
    gl_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gl_vertex_shader, 1, &raw_str, nullptr);

    glCompileShader(gl_vertex_shader);
    glAttachShader(gl_program, gl_vertex_shader);

    glObjectLabel(GL_SHADER, gl_vertex_shader, -1, vertex_filepath.c_str());

    glGetShaderiv(gl_vertex_shader, GL_COMPILE_STATUS, &vertex_shader_compiled);
  }

  if (!geometry_filepath.empty()) {
    if (geometry_src.empty()) {
      return {false, "Geometry shader passed could not be opened or is empty"};
    }

    for (const auto &define : defines) {
      geometry_src.insert(0, shader_define_to_string(define));
    }

    geometry_src.insert(0, GLSL_VERSION);

    const auto raw_str = geometry_src.c_str();
    gl_geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(gl_geometry_shader, 1, &raw_str, nullptr);

    glCompileShader(gl_geometry_shader);
    glAttachShader(gl_program, gl_geometry_shader);

    glObjectLabel(GL_SHADER, gl_geometry_shader, -1, geometry_filepath.c_str());

    glGetShaderiv(gl_geometry_shader, GL_COMPILE_STATUS,
                  &geometry_shader_compiled);
  }

  if (!fragment_filepath.empty()) {
    if (fragment_src.empty()) {
      return {false, "Fragment shader passed could not be opened or is empty"};
    }

    for (const auto &define : defines) {
      fragment_src.insert(0, shader_define_to_string(define));
    }

    fragment_src.insert(0, GLSL_VERSION);

    const auto raw_str = fragment_src.c_str();
    gl_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gl_fragment_shader, 1, &raw_str, nullptr);

    glCompileShader(gl_fragment_shader);
    glAttachShader(gl_program, gl_fragment_shader);

    glObjectLabel(GL_SHADER, gl_fragment_shader, -1, fragment_filepath.c_str());

    glGetShaderiv(gl_fragment_shader, GL_COMPILE_STATUS,
                  &fragment_shader_compiled);
  }

  const std::string program_label =
      (vertex_included ? vertex_filepath + ", " : "") +
      (geometry_included ? geometry_filepath + ", " : "") +
      (fragment_included ? fragment_filepath + ", " : "");
  glObjectLabel(GL_PROGRAM, gl_program, -1, program_label.c_str());

  const bool all_shaders_compiled =
      (vertex_included ? vertex_shader_compiled : true) &&
      (geometry_included ? geometry_shader_compiled : true) &&
      (fragment_included ? fragment_shader_compiled : true);

  if (!all_shaders_compiled) {
    std::string total_err_msg = "";

    if (vertex_included) {
      total_err_msg += "Vertex shader (" + vertex_filepath + "): \n" +
                       shader_compilation_err_msg(gl_vertex_shader);
      glDetachShader(gl_program, gl_vertex_shader);
      glDeleteShader(gl_vertex_shader);
    }

    if (geometry_included) {
      total_err_msg += "Geometry shader (" + geometry_filepath + "): \n" +
                       shader_compilation_err_msg(gl_geometry_shader);
      glDetachShader(gl_program, gl_geometry_shader);
      glDeleteShader(gl_geometry_shader);
    }

    if (fragment_included) {
      total_err_msg += "Fragment shader (" + fragment_filepath + "): \n" +
                       shader_compilation_err_msg(gl_fragment_shader);
      glDetachShader(gl_program, gl_fragment_shader);
      glDeleteShader(gl_fragment_shader);
    }

    GLint err_size = 0;
    glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &err_size);
    char err_msg[err_size];
    glGetProgramInfoLog(gl_program, err_size, nullptr, err_msg);
    total_err_msg += std::string(err_msg);

    glDeleteProgram(gl_program);

    return {false, total_err_msg};
  }

  glLinkProgram(gl_program);

  if (vertex_included) {
    glDetachShader(gl_program, gl_vertex_shader);
    glDeleteShader(gl_vertex_shader);
  }

  if (geometry_included) {
    glDetachShader(gl_program, gl_geometry_shader);
    glDeleteShader(gl_geometry_shader);
  }

  if (fragment_included) {
    glDetachShader(gl_program, gl_fragment_shader);
    glDeleteShader(gl_fragment_shader);
  }

  return {true, ""};
};

void Shader::add(const Shader::Defines define) { defines.insert(define); }

void Shader::add(const std::string &str) {
  if (!vertex_filepath.empty()) {
    vertex_src.insert(0, str);
  }

  if (!geometry_filepath.empty()) {
    geometry_src.insert(0, str);
  }

  if (!fragment_filepath.empty()) {
    fragment_src.insert(0, str);
  }
}

bool Shader::operator==(const Shader &rhs) { return defines == rhs.defines; }
