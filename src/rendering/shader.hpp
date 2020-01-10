#pragma once
#ifndef MEINEKRAFT_SHADER_HPP
#define MEINEKRAFT_SHADER_HPP

#include <set>
#include <string>
#include <vector>

struct Shader {

  enum class Defines : uint32_t {
    Diffuse2D,      // OpenGL texture target (GL_TEXTURE_2D)
    DiffuseCubemap, // OpenGL texture target (GL_TEXTURE_CUBE_MAP)
    DiffuseRGB,     // Diffuse texture format 24 bit depth
    DiffuseRGBA,    // Diffuse texture format 32 bit depth
    Emissive        // Has emissive texture
  };

  Shader() = default;
  Shader(const std::string &vert_shader_file,
         const std::string &frag_shader_file);
  Shader(const std::string &vert_shader_file,
         const std::string &geom_shader_file,
         const std::string &frag_shader_file);

  // Add shader config. definition
  void add(const Shader::Defines define);

  // Adds raw string to the shader sources
  void add(const std::string &str);

  /// Loads and compiles shader sources returns compile error msg
  std::pair<bool, std::string> compile();

  // Filepaths
  // NOTE: Shader stage considered active if it is not .empty()
  std::string vertex_filepath = "";
  std::string geometry_filepath = "";
  std::string fragment_filepath = "";


  uint32_t gl_program = 0;
  uint32_t gl_vertex_shader = 0;
  uint32_t gl_geometry_shader = 0;
  uint32_t gl_fragment_shader = 0;

  // Set by compile function, gets stale if the shader is changed
  bool compiled_successfully = false;

  bool operator==(const Shader &rhs);

  // Configuration of the shader a la Ubershader
  std::set<Shader::Defines> defines{};

private:
  // Shader source to be added
  std::string include_vertex_src = "";
  std::string include_geometry_src = "";
  std::string include_fragment_src = "";
};

struct ComputeShader {
  explicit ComputeShader(const std::string &compute_filepath,
                         const std::vector<std::string> &defines = {});
  uint32_t gl_program = 0;
};

#endif // MEINEKRAFT_SHADER_HPP
