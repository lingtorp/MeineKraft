#ifndef MEINEKRAFT_TEXTURE_H
#define MEINEKRAFT_TEXTURE_H

#include <string>
#include <vector>

enum class FileExtension {
    png, jpg, unknown
};

class Texture {
public:
    Texture();

    bool load(std::string filepath);
    bool load_1d(std::string filepath);
    bool load_2d(std::string filepath);
    bool load_cube_map(std::vector<std::string> faces);

    uint64_t gl_texture;
    uint64_t gl_texture_type;
    uint64_t gl_texture_location;

    bool loaded_successfully;

    /// Function of the texture
    enum class Type {
        Diffuse, Specular
    };

private:
    uint64_t load_1d_texture(std::string filepath);
    uint64_t load_2d_texture(std::string filepath);
    uint64_t load_cube_map(std::vector<std::string> faces, FileExtension file_extension);

    /// Conversion methods from and to FileExtension and their OpenGL counterparts
    FileExtension file_format_from_file_at(std::string filepath);
    uint64_t gl_format_from_file_extension(FileExtension file_extension);
};

#endif //MEINEKRAFT_TEXTURE_H
