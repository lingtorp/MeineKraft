#pragma once
#ifndef MEINEKRAFT_SHADER_HPP
#define MEINEKRAFT_SHADER_HPP

#include <string>
#include <vector>
#include <set>

struct Shader {
  enum class Defines: uint32_t {
    Diffuse2D,        // OpenGL texture target (GL_TEXTURE_2D)
    DiffuseCubemap,   // OpenGL texture target (GL_TEXTURE_CUBE_MAP)
    DiffuseRGB,       // Diffuse texture format 24 bit depth
    DiffuseRGBA       // Diffuse texture format 32 bit depth
  };
  // Configuration of the shader a la Ubershader
  std::set<Shader::Defines> defines;

  Shader() = default;
  Shader(const std::string& vert_shader_file,
         const std::string& frag_shader_file);
  Shader(const std::string& vert_shader_file,
         const std::string& geom_shader_file,
         const std::string& frag_shader_file);

  bool operator==(const Shader& rhs);

  /// Loads and compiles the shader source, return compile error message in the pair.
  std::pair<bool, std::string> compile();

  static std::string shader_define_to_string(Shader::Defines define);
  void add(Shader::Defines define);

  std::string vertex_filepath;
  std::string fragment_filepath;
  std::string vertex_src;
  std::string fragment_src;
  
  uint32_t gl_program = 0;
  uint32_t gl_vertex_shader   = 0;
  uint32_t gl_geometry_shader = 0;
  uint32_t gl_fragment_shader = 0;

  bool compiled_successfully = false;
};

struct ComputeShader {
  ComputeShader(const std::string& compute_filepath,
                const std::vector<std::string>& defines = {});
  uint32_t gl_program;
};

#endif // MEINEKRAFT_SHADER_HPP
