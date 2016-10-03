#ifndef MEINEKRAFT_SHADER_H
#define MEINEKRAFT_SHADER_H

#include <string>

class Shader {
public:
    Shader(std::string vertex_filepath, std::string fragment_filepath);

    std::pair<bool, std::string> compile();

    std::string vertex_filepath;
    std::string fragment_filepath;
    uint64_t gl_program;

private:
    const std::string load_shader_source(std::string filename) const;
};

#endif //MEINEKRAFT_SHADER_H
