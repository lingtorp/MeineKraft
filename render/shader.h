#ifndef MEINEKRAFT_SHADER_H
#define MEINEKRAFT_SHADER_H

#include <string>

class Shader {
public:
    Shader(std::string vertex_filepath, std::string fragment_filepath);

    /// Loads and compiles the shader source, return compile error message in the pair.
    std::pair<bool, std::string> compile();

    /// Loads and recompiles both shaders
    std::pair<bool, std::string> recompile();

    std::string vertex_filepath;
    std::string fragment_filepath;
    uint64_t gl_program;

private:
    uint64_t vertex_shader;
    uint64_t fragment_shader;

    /// Assumes the file exists and will seg. fault otherwise.
    const std::string load_shader_source(std::string filename) const;
};

#endif //MEINEKRAFT_SHADER_H
