#pragma once
#ifndef MEINEKRAFT_SHADER_HPP
#define MEINEKRAFT_SHADER_HPP

#include <set>
#include <string>
#include <vector>

/// Shader implementation is meant to be used immutable.
/// Load the shader files needed, append some includes on them and compile.
/// It is an error to compile a Shader twice if it has succeded.
struct Shader {

  /// Shader definition for various types of textures handled
  enum class Defines : uint32_t {
    Diffuse2D,      // OpenGL texture target (GL_TEXTURE_2D)
    DiffuseCubemap, // OpenGL texture target (GL_TEXTURE_CUBE_MAP)
    DiffuseRGB,     // Diffuse texture format 24 bit depth
    DiffuseRGBA,    // Diffuse texture format 32 bit depth
    DiffuseScalars, // Diffuse scalars instead of texture
    Emissive,       // Has emissive texture
    EmissiveScalars // Emissive scalars instead of texture
  };

  Shader() = default;
  Shader(const std::string &vert_shader_file,
         const std::string &frag_shader_file);
  Shader(const std::string &vert_shader_file,
         const std::string &geom_shader_file,
         const std::string &frag_shader_file);

  /// Add shader config. definition, returns whether or not it was successful
  bool add(const Shader::Defines define);

  /// Adds raw string to the shader sources, returns whether or not it was successful
  bool add(const std::string &str);

  /// Loads and compiles shader sources returns compile error msg
  std::pair<bool, std::string> compile();

  /// Equality operator according to the unique defines
  bool operator==(const Shader &rhs);

  /// Filepaths
  /// NOTE: Shader stage considered active if it is not .empty()
  std::string vertex_filepath = "";
  std::string geometry_filepath = "";
  std::string fragment_filepath = "";

  /// OpenGL object identifiers
  uint32_t gl_program = 0;
  uint32_t gl_vertex_shader = 0;
  uint32_t gl_geometry_shader = 0;
  uint32_t gl_fragment_shader = 0;

  /// Configuration of the shader a la Ubershader
  std::set<Shader::Defines> defines{};

private:
  /// Set by compile()
  bool compiled_successfully = false;

  /// Shader source to be added
  std::string include_vertex_src = "";
  std::string include_geometry_src = "";
  std::string include_fragment_src = "";
};

/// Defines and compiles a compute shader in one go in contrast to 'struct Shader'
/// See documentation for 'struct Shader'
struct ComputeShader {
  explicit ComputeShader(const std::string &compute_filepath,
                         const std::vector<std::string> &defines = {});
  uint32_t gl_program = 0;
};

#endif // MEINEKRAFT_SHADER_HPP
