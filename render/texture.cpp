#include "texture.h"

#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <SDL_log.h>
#include <assert.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>

Texture::Texture(): gl_texture(0), gl_texture_type(0), gl_texture_location(0), loaded_successfully(false) {}

FileExtension Texture::file_format_from_file_at(std::string filepath) {
    std::string extension{SDL_strrchr(filepath.c_str(), '.')};
    if (extension == ".png") {
        return FileExtension::png;
    } else if (extension == ".jpg") {
        return FileExtension::jpg;
    } else {
        return FileExtension::unknown;
    }
}

uint64_t Texture::gl_format_from_file_extension(FileExtension file_extension) {
    switch (file_extension) {
        case FileExtension::png:
            return GL_RGBA;
        case FileExtension ::jpg:
            return GL_RGB;
        default:
            return GL_RGBA;
    }
}

/// Texture loading order; right, left, top, bottom, back, front
uint64_t Texture::load_cube_map(std::vector<std::string> faces, FileExtension file_extension) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    GLint internal_format = (GLint) gl_format_from_file_extension(file_extension);

    int i = 0;
    for (auto filepath : faces) {
        SDL_Surface *image = IMG_Load(filepath.c_str());
        if (image == nullptr) { continue; }
        int width  = image->w;
        int height = image->h;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, image->pixels);
        SDL_FreeSurface(image);
        i++;
    }
    return texture;
}

/// Loads a 2D texture from filepath with the corresponding fileformat
uint64_t Texture::load_2d_texture(std::string filepath) {
    assert(filepath.size() > 0);
    SDL_Surface *image = IMG_Load(filepath.c_str());
    if (!image) { SDL_Log("%s", IMG_GetError()); }
    int width  = image->w;
    int height = image->h;
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    auto file_format = file_format_from_file_at(filepath);
    if (file_format == FileExtension::unknown) { SDL_Log("%s %s", "Invalid file type:", filepath.c_str());}
    GLuint internal_format = (GLint) gl_format_from_file_extension(file_format);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, image->pixels);
    SDL_FreeSurface(image);
    return texture;
}

uint64_t Texture::load_1d_texture(std::string filepath) {
    assert(filepath.size() > 0);
    SDL_Surface *image = IMG_Load(filepath.c_str());
    if (!image) { SDL_Log("%s", IMG_GetError()); }
    int width  = image->w;
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_1D, texture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_1D);
    auto file_format = file_format_from_file_at(filepath);
    if (file_format == FileExtension::unknown) { SDL_Log("%s %s", "Invalid file type:", filepath.c_str()); }
    GLuint internal_format = (GLint) gl_format_from_file_extension(file_format);
    glTexImage1D(GL_TEXTURE_1D, 0, internal_format, width, 0, internal_format, GL_UNSIGNED_BYTE, image->pixels);
    SDL_FreeSurface(image);
    return texture;
}

bool Texture::load_cube_map(std::vector<std::string> faces) {
    if (faces.size() != 6) { return false; }
    auto file_extension = file_format_from_file_at(faces[0]);
    if (file_extension == FileExtension::unknown) { return false; }
    gl_texture = load_cube_map(faces, file_extension);
    gl_texture_type = GL_TEXTURE_CUBE_MAP;
    loaded_successfully = true;
    return loaded_successfully;
}

bool Texture::load_2d(std::string filepath) {
    auto file_extension = file_format_from_file_at(filepath);
    if (file_extension == FileExtension::unknown) { return false; }
    gl_texture = load_2d_texture(filepath);
    gl_texture_type = GL_TEXTURE_2D;
    loaded_successfully = true;
    return loaded_successfully;
}

bool Texture::load_1d(std::string filepath) {
    auto file_extension = file_format_from_file_at(filepath);
    if (file_extension == FileExtension::unknown) { return false; }
    gl_texture = load_1d_texture(filepath);
    gl_texture_type = GL_TEXTURE_1D;
    loaded_successfully = true;
    return loaded_successfully;
}

bool Texture::load(std::string filepath) {
    auto file_extension = file_format_from_file_at(filepath);
    if (file_extension == FileExtension::unknown) { return false; }
    SDL_Surface *image = IMG_Load(filepath.c_str());
    if (!image) {
        SDL_Log("%s", IMG_GetError());
        return false;
    }
    if (image->h > 1) {
        gl_texture = load_2d_texture(filepath);
        gl_texture_type = GL_TEXTURE_2D;
    } else {
        gl_texture = load_1d_texture(filepath);
        gl_texture_type = GL_TEXTURE_1D;
    }
    loaded_successfully = true;
    return loaded_successfully;
}
