#include "shader.h"

#include <GL/glew.h>
#include <iostream>
#include <fstream>

Shader::Shader(std::string vertex_filepath, std::string fragment_filepath): vertex_filepath(vertex_filepath),
                                                                            fragment_filepath(fragment_filepath),
                                                                            gl_program(0) {}

/// Loads and compiles the shader source, return compile error message in the pair.
std::pair<bool, std::string> Shader::compile() {
    auto vertex_src   = load_shader_source(vertex_filepath);
    auto fragment_src = load_shader_source(fragment_filepath);

    auto raw_str = vertex_src.c_str();
    uint64_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &raw_str, NULL);

    raw_str = fragment_src.c_str();
    uint64_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &raw_str, NULL);

    uint64_t shader_program = glCreateProgram();
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint vertex_shader_status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_status);

    GLint fragment_shader_status;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_shader_status);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (vertex_shader_status == 1 && fragment_shader_status == 1) {
        gl_program = shader_program;
        return {true, ""};
    }

    uint64_t err_size = 512;
    std::string frag_err_msg = "";
    std::string vert_err_msg = "";
    frag_err_msg.reserve(err_size);
    vert_err_msg.reserve(err_size);
    glGetShaderInfoLog(fragment_shader, err_size, NULL, (char *) frag_err_msg.c_str());
    glGetShaderInfoLog(vertex_shader  , err_size, NULL, (char *) vert_err_msg.c_str());
    return {false, frag_err_msg + vert_err_msg};
};

/// Assumes the file exists and will seg. fault otherwise.
const std::string Shader::load_shader_source(std::string filename) const {
    std::ifstream ifs(filename);
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}
