#include "shader.hpp"

#include "../util/filesystem.hpp"
#include "debug_opengl.hpp"

#include <cassert>
#include <string>

#ifdef _WIN32
#include <glew.h>
#else
#include <GL/glew.h>
#endif

#if defined(__linux__)
static const char *GLSL_VERSION = "#version 450 core \n";
#elif defined(WIN32)
static const char *GLSL_VERSION = "#version 460 core \n";
#endif

/// Maps shader config flags to shader source code defines as used in the ubershaders
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
  case Shader::Defines::DiffuseScalars:
    return "#define DIFFUSE_SCALARS \n";
    break;
  case Shader::Defines::Emissive:
    return "#define HAS_EMISSIVE_TEXTURE \n";
    break;
  case Shader::Defines::EmissiveScalars:
    return "#define HAS_EMISSIVE_SCALARS \n";
    break;
  default:
    Log::error("Invalid shader define passed");
    return "This will ensure that shader compilation does not work :)";
  }
}

/// NOTE: Tries to parse out where and what went wrong in the shader
static std::string try_to_parse_shader_err_msg(const std::string& shader_src, const std::string& err_msg) {
  return err_msg; // TODO: Implement ...
}

static std::string shader_compilation_err_msg(const GLuint shader) {
  GLint max_lng = 0; // max_lng includes the NULL character
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_lng);

  if (max_lng == 0) {
    return "";
  }

  char err_log[max_lng];
  glGetShaderInfoLog(shader, max_lng, &max_lng, &err_log[0]);

  return std::string(&err_log[0]);
}

// TODO: Rewrite to use a compile function for correct err msg handling (remove
// exit(-1))
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

  comp_src.insert(0, GLSL_VERSION);

  const char *raw_str_ptr = comp_src.c_str();
  glShaderSource(gl_comp_shader, 1, &raw_str_ptr, nullptr);
  glCompileShader(gl_comp_shader);

  GLint compute_shader_status = 0;
  glGetShaderiv(gl_comp_shader, GL_COMPILE_STATUS, &compute_shader_status);

  if (!compute_shader_status) {
    const std::string err_msg = shader_compilation_err_msg(gl_comp_shader);
    Log::error("Could not compile compute shader: " + compute_filepath + "\n" +
               err_msg);
    glDetachShader(gl_program, gl_comp_shader);
    glDeleteShader(gl_comp_shader);
    exit(-1);
  }

  glObjectLabel(GL_SHADER, gl_comp_shader, -1, compute_filepath.c_str());

  gl_program = glCreateProgram();
  glAttachShader(gl_program, gl_comp_shader);
  glLinkProgram(gl_program);

  GLint program_linked = 0;
  glGetProgramiv(gl_program, GL_LINK_STATUS, &program_linked);

  if (!program_linked) {
    GLint err_size = 0;
    glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &err_size);
    char program_err_msg[err_size];
    glGetProgramInfoLog(gl_program, err_size, nullptr, program_err_msg);
    Log::warn(try_to_parse_shader_err_msg(comp_src, std::string(program_err_msg)));

    glDeleteProgram(gl_program);
  }

  glObjectLabel(GL_PROGRAM, gl_program, -1, compute_filepath.c_str());

  glDetachShader(gl_program, gl_comp_shader);
  glDeleteShader(gl_comp_shader);
}

Shader::Shader(const std::string &vertex_filepath,
               const std::string &geometry_filepath,
               const std::string &fragment_filepath)
    : vertex_filepath(vertex_filepath), geometry_filepath(geometry_filepath),
      fragment_filepath(fragment_filepath) {}

Shader::Shader(const std::string &vertex_filepath,
               const std::string &fragment_filepath)
    : vertex_filepath(vertex_filepath), fragment_filepath(fragment_filepath) {}

std::pair<bool, std::string> Shader::compile() {
  if (compiled_successfully) {
    return {false, "This shader has already been compiled."};
  }

  const bool vertex_included = !vertex_filepath.empty();
  const bool geometry_included = !geometry_filepath.empty();
  const bool fragment_included = !fragment_filepath.empty();

  gl_program = glCreateProgram();

  GLint vertex_shader_compiled = GL_FALSE;
  GLint geometry_shader_compiled = GL_FALSE;
  GLint fragment_shader_compiled = GL_FALSE;

  if (!vertex_filepath.empty()) {
    std::string vertex_src = Filesystem::read_file(vertex_filepath);

    if (vertex_src.empty()) {
      return {false, "Vertex shader passed could not be opened or is empty"};
    }

    for (const auto &define : defines) {
      vertex_src.insert(0, shader_define_to_string(define));
    }

    vertex_src.insert(0, include_vertex_src);
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
    std::string geometry_src = Filesystem::read_file(geometry_filepath);

    if (geometry_src.empty()) {
      return {false, "Geometry shader passed could not be opened or is empty"};
    }

    for (const auto &define : defines) {
      geometry_src.insert(0, shader_define_to_string(define));
    }

    geometry_src.insert(0, include_geometry_src);
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
    std::string fragment_src = Filesystem::read_file(fragment_filepath);

    if (fragment_src.empty()) {
      return {false, "Fragment shader passed could not be opened or is empty"};
    }

    for (const auto &define : defines) {
      fragment_src.insert(0, shader_define_to_string(define));
    }

    fragment_src.insert(0, include_fragment_src);
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

  std::string err_msg = "";
  GLint program_linked = 0;
  glGetProgramiv(gl_program, GL_LINK_STATUS, &program_linked);

  if (!program_linked) {
    GLint err_size = 0;
    glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &err_size);
    char program_err_msg[err_size];
    glGetProgramInfoLog(gl_program, err_size, nullptr, program_err_msg);
    err_msg += "Program error (" + vertex_filepath + ", " + geometry_filepath +
               ", " + fragment_filepath + "): \n" +
               std::string(program_err_msg);
  }

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

  if (!program_linked) {
    glDeleteProgram(gl_program);
  }

  compiled_successfully = (program_linked == GL_TRUE);
  return {compiled_successfully, err_msg};
};

bool Shader::add(const Shader::Defines define) {
  if (!compiled_successfully) {
    defines.insert(define);
  }
  return !compiled_successfully;
}

bool Shader::add(const std::string &str) {
  assert(!str.empty() && "Passed empty shader string to include ...");

  if (compiled_successfully) {
    return false;
  }

  if (!vertex_filepath.empty()) {
    include_vertex_src.insert(0, str);
  }

  if (!geometry_filepath.empty()) {
    include_geometry_src.insert(0, str);
  }

  if (!fragment_filepath.empty()) {
    include_fragment_src.insert(0, str);
  }

  return true;
}

bool Shader::operator==(const Shader &rhs) { return defines == rhs.defines; }
