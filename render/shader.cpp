#include "shader.h"

#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <SDL_log.h>

Shader::Shader(std::string vertex_filepath, std::string fragment_filepath): vertex_filepath(vertex_filepath),
                                                                            fragment_filepath(fragment_filepath),
                                                                            vertex_shader(0),
                                                                            fragment_shader(0),
                                                                            gl_program(0) {}

std::pair<bool, std::string> Shader::compile() {
    auto vertex_src   = load_shader_source(vertex_filepath);
    auto fragment_src = load_shader_source(fragment_filepath);

    auto raw_str = vertex_src.c_str();
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &raw_str, NULL);

    raw_str = fragment_src.c_str();
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
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

    // Always detach shaders after a successful link.
    glDetachShader(gl_program, vertex_shader);
    glDetachShader(gl_program, fragment_shader);
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

std::pair<bool, std::string> Shader::recompile() {
    auto vertex_src   = load_shader_source(vertex_filepath);
    auto fragment_src = load_shader_source(fragment_filepath);

    glDetachShader(gl_program, vertex_shader);
    glDetachShader(gl_program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    vertex_shader   = glCreateShader(GL_VERTEX_SHADER);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    auto raw_str = vertex_src.c_str();
    glShaderSource(vertex_shader, 1, &raw_str, NULL);

    raw_str = fragment_src.c_str();
    glShaderSource(fragment_shader, 1, &raw_str, NULL);

    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    GLint vertex_shader_status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_status);

    GLint fragment_shader_status;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_shader_status);

    if (vertex_shader_status == 1 && fragment_shader_status == 1) {
        // Relink shader program
        glAttachShader(gl_program, vertex_shader);
        glAttachShader(gl_program, fragment_shader);
        glLinkProgram(gl_program);
        GLint is_linked = 0;
        glGetProgramiv(gl_program, GL_LINK_STATUS, &is_linked);
        SDL_Log("Shader relinking is success: %i", is_linked == GL_TRUE);

        if (is_linked) {
            // Always detach shaders after a successful link.
            glDetachShader(gl_program, vertex_shader);
            glDetachShader(gl_program, fragment_shader);
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
        }

        std::string err_log = "";
        GLint max_log_lng = 0;
        glGetProgramiv(gl_program, GL_INFO_LOG_LENGTH, &max_log_lng);
        err_log.reserve(max_log_lng);
        glGetProgramInfoLog(gl_program, max_log_lng, NULL, (char *) err_log.c_str());
        SDL_Log("%s", err_log.c_str());

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

const std::string Shader::load_shader_source(std::string filename) const {
    std::ifstream ifs(filename);
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}
