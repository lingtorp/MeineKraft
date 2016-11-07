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

    /// Returns the OpenGL texture id
    uint64_t load(std::string filepath, std::string directory);

    uint64_t gl_texture;
    uint64_t gl_texture_type;
    uint64_t gl_texture_location;
    bool loaded_succesfully;

private:
    /// Returns a default texture when no texture could be loaded for various reasons
    uint64_t default_texture();
    uint64_t load_1d_texture(std::string filepath);
    uint64_t load_2d_texture(std::string filepath);
    uint64_t load_cube_map(std::vector<std::string> faces, FileExtension file_format);
    FileExtension file_format_from_file_at(std::string filepath);
    uint64_t gl_format_from_file_extension(FileExtension file_format);
};

#endif //MEINEKRAFT_TEXTURE_H
