#include "shader.hpp"

#include "../util/filesystem.hpp"

#include <cassert>
#include <string>

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
  case Shader::Defines::TangentNormals:
    return "#define HAS_TANGENT_NORMALS \n";
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

static std::string shader_compilation_err_msg(const uint32_t shader) {
  // TODO
  return "";
}

ComputeShader::ComputeShader(const std::string &compute_filepath,
                             const std::vector<std::string> &defines) {
  // TODO
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
  // TODO
  return {false, ""};
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
