#include "render.h"

#include <GL/glew.h>
#include <SDL2/SDL_image.h>
#include <array>

#include "../math/vector.h"
#include "primitives.h"
#include "../world/world.h"
#include "camera.h"

#include "graphicsbatch.h"
#include "rendercomponent.h"
#include "../nodes/entity.h"
#include "shader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../include/tinyobjloader/tiny_obj_loader.h"

typedef enum { png, jpg } FileFormat;

/// Texture loading order; right, left, top, bottom, back, front
GLuint load_cube_map(std::vector<std::string> faces, FileFormat file_format) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    GLint internalFormat;
    switch (file_format) {
        case png:
            internalFormat = GL_RGBA;
            break;
        case jpg:
            internalFormat = GL_RGB;
            break;
        default:
            internalFormat = GL_RGBA;
    }

    int i = 0;
    for (auto filepath : faces) {
        SDL_Surface *image = IMG_Load(filepath.c_str());
        int width = image->w;
        int height = image->h;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width,
                     height, 0, internalFormat, GL_UNSIGNED_BYTE, image->pixels);
        free(image);
        i++;
    }
    return texture;
}

/// Column major - Camera combined rotation matrix (y, x) & translation matrix
Mat4<float> Renderer::FPSViewRH(Vec3<float> eye, float pitch, float yaw) {
    static constexpr float rad = M_PI / 180;
    float cosPitch = cosf(pitch * rad);
    float sinPitch = sinf(pitch * rad);
    float cosYaw = cosf(yaw * rad);
    float sinYaw = sinf(yaw * rad);
    auto xaxis = Vec3<float>{cosYaw, 0, -sinYaw};
    auto yaxis = Vec3<float>{sinYaw * sinPitch, cosPitch, cosYaw * sinPitch};
    auto zaxis = Vec3<float>{sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};
    Mat4<float> matrix;
    matrix[0][0] = xaxis.x;
    matrix[0][1] = yaxis.x;
    matrix[0][2] = zaxis.x;
    matrix[0][3] = 0.0f;
    matrix[1][0] = xaxis.y;
    matrix[1][1] = yaxis.y;
    matrix[1][2] = zaxis.y;
    matrix[1][3] = 0.0f;
    matrix[2][0] = xaxis.z;
    matrix[2][1] = yaxis.z;
    matrix[2][2] = zaxis.z;
    matrix[2][3] = 0.0f;
    matrix[3][0] = -xaxis.dot(eye);
    matrix[3][1] = -yaxis.dot(eye);
    matrix[3][2] = -zaxis.dot(eye); // GLM says no minus , other's say minus
    matrix[3][3] = 1.0f;
    return matrix;
}

/// A.k.a perspective matrix
Mat4<float> gen_projection_matrix(float z_near, float z_far, float fov, float aspect) {
    float rad = M_PI / 180;
    float tanHalf = tanf(fov * rad / 2);
    float a = 1 / (tanHalf * aspect);
    float b = 1 / tanHalf;
    float c = -(z_far + z_near) / (z_far - z_near);
    float d = -(2 * z_far * z_near) / (z_far - z_near);
    Mat4<float> matrix;
    matrix[0] = {a, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, b, 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, c, d};
    matrix[3] = {0.0f, 0.0f, -1.0f, 0.0f};
    return matrix;
}

Renderer::Renderer(): DRAW_DISTANCE(200), projection_matrix(Mat4<float>()), state{}, graphics_batches{}, shaders{}, render_components_id(0) {
    glewExperimental = (GLboolean) true;
    glewInit();

    /// Load all block & skybox textures
    auto base = "/Users/AlexanderLingtorp/Google Drive/Repositories/MeineKraft/";
    std::vector<std::string> cube_faces = {base + std::string("res/blocks/grass/side.jpg"),
                                           base + std::string("res/blocks/grass/side.jpg"),
                                           base + std::string("res/blocks/grass/top.jpg"),
                                           base + std::string("res/blocks/grass/bottom.jpg"),
                                           base + std::string("res/blocks/grass/side.jpg"),
                                           base + std::string("res/blocks/grass/side.jpg")};
    this->textures[Texture::GRASS] = load_cube_map(cube_faces, jpg);

    std::vector<std::string> skybox_faces = {base + std::string("res/sky/right.jpg"),
                                             base + std::string("res/sky/left.jpg"),
                                             base + std::string("res/sky/top.jpg"),
                                             base + std::string("res/sky/bottom.jpg"),
                                             base + std::string("res/sky/back.jpg"),
                                             base + std::string("res/sky/front.jpg")};
    this->textures[Texture::SKYBOX] = load_cube_map(skybox_faces, jpg);

    /// Compile shaders
    // Files are truly horrible and filepaths are even worse, therefore this is not scalable
    std::string shader_base_filepath = "/Users/AlexanderLingtorp/Google Drive/Repositories/MeineKraft/shaders/";
    std::string err_msg;
    bool success;

    auto skybox_vert = shader_base_filepath + "skybox/vertex-shader.glsl";
    auto skybox_frag = shader_base_filepath + "skybox/fragment-shader.glsl";
    Shader skybox_shader(skybox_vert, skybox_frag);
    std::tie(success, err_msg) = skybox_shader.compile();
    shaders.insert({ShaderType::SKYBOX_SHADER, skybox_shader});
    if (!success) { SDL_Log("%s", err_msg.c_str()); }

    auto std_vert = shader_base_filepath + "block/vertex-shader.glsl";
    auto std_frag = shader_base_filepath + "block/fragment-shader.glsl";
    Shader std_shader(std_vert, std_frag);
    std::tie(success, err_msg) = std_shader.compile();
    shaders.insert({ShaderType::STANDARD_SHADER, std_shader});
    if (!success) { SDL_Log("%s", err_msg.c_str()); }

    // Camera
    auto position  = Vec3<float>{0.0f, 0.0f, 0.0f};  // cam position
    auto direction = Vec3<float>{0.0f, 0.0f, -1.0f}; // position of where the cam is looking
    auto world_up  = Vec3<float>{0.0, 1.0f, 0.0f};   // world up
    this->camera = std::make_shared<Camera>(position, direction, world_up);
}

bool Renderer::point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes) {
    auto left_plane = planes[0]; auto right_plane = planes[1];
    auto top_plane  = planes[2]; auto bot_plane   = planes[3];
    auto near_plane = planes[4]; auto far_plane   = planes[5];
    auto dist_l = left_plane.distance_to_point(point);
    auto dist_r = right_plane.distance_to_point(point);
    auto dist_t = top_plane.distance_to_point(point);
    auto dist_b = bot_plane.distance_to_point(point);
    auto dist_n = near_plane.distance_to_point(point);
    auto dist_f = far_plane.distance_to_point(point);
    if (dist_l < 0 && dist_r < 0 && dist_t < 0 && dist_b < 0 && dist_n < 0 && dist_f < 0) { return true; }
    return false;
}

void Renderer::render() {
    /// Reset render stats
    state = RenderState();

    /// Frustrum planes
    auto camera_view = FPSViewRH(camera->position, camera->pitch, camera->yaw);
    auto frustrum_view = camera_view * projection_matrix;
    std::array<Plane<float>, 6> planes = extract_planes(frustrum_view.transpose());
    auto left_plane = planes[0]; auto right_plane = planes[1];
    auto top_plane  = planes[2]; auto bot_plane   = planes[3];
    auto near_plane = planes[4]; auto far_plane   = planes[5];

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    for (auto batch : graphics_batches) {
        glBindVertexArray(batch.gl_VAO);
        glUseProgram(batch.gl_shader_program);
        glUniformMatrix4fv(batch.gl_camera_view, 1, GL_FALSE, camera_view.data());

        // TODO: Setup glEnables and stuff, gotta set the defaults in GraphicsState too
//        glEnable(GL_CULL_FACE);
//        glCullFace(GL_FRONT);
//        glFrontFace(GL_CCW);

        std::vector<Mat4<float>> buffer{};
        for (auto component : batch.components) {
            // Frustrum cullling
            if (point_inside_frustrum(component.entity->position, planes)) { continue; }

            // Draw distance
            auto camera_to_cube = camera->position - component.entity->position;
            if (camera_to_cube.length() >= DRAW_DISTANCE) { continue; }

            glBindTexture(GL_TEXTURE_CUBE_MAP, textures[component.graphics_state.gl_texture]);

            Mat4<float> model{};
            model = model.translate(component.entity->position);
            model = model.scale(component.entity->scale);
            model = model.transpose();
            buffer.push_back(model);
        }
        glBindBuffer(GL_ARRAY_BUFFER, batch.gl_models_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(Mat4<float>), buffer.data(), GL_DYNAMIC_DRAW);
        glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, 0, buffer.size());

        /// Update render stats
        state.entities += buffer.size();
    }
    state.graphic_batches = graphics_batches.size();
}

Renderer::~Renderer() {
    // TODO: Clear up all the GraphicsObjects
}

/// Returns the planes from the frustrum matrix in order; {left, right, bottom, top, near, far}
/// See: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
std::array<Plane<float>, 6> Renderer::extract_planes(Mat4<float> mat) {
    auto planes = std::array<Plane<float>, 6>();
    auto left_plane  = Plane<float>(mat[3][0] + mat[0][0],
                                    mat[3][1] + mat[0][1],
                                    mat[3][2] + mat[0][2],
                                    mat[3][3] + mat[0][3]);

    auto right_plane = Plane<float>(mat[3][0] - mat[0][0],
                                    mat[3][1] - mat[0][1],
                                    mat[3][2] - mat[0][2],
                                    mat[3][3] - mat[0][3]);

    auto bot_plane   = Plane<float>(mat[3][0] + mat[1][0],
                                    mat[3][1] + mat[1][1],
                                    mat[3][2] + mat[1][2],
                                    mat[3][3] + mat[1][3]);

    auto top_plane   = Plane<float>(mat[3][0] - mat[1][0],
                                    mat[3][1] - mat[1][1],
                                    mat[3][2] - mat[1][2],
                                    mat[3][3] - mat[1][3]);

    auto near_plane  = Plane<float>(mat[3][0] + mat[2][0],
                                    mat[3][1] + mat[2][1],
                                    mat[3][2] + mat[2][2],
                                    mat[3][3] + mat[2][3]);

    auto far_plane   = Plane<float>(mat[3][0] - mat[2][0],
                                    mat[3][1] - mat[2][1],
                                    mat[3][2] - mat[2][2],
                                    mat[3][3] - mat[2][3]);
    planes[0] = left_plane; planes[1] = right_plane; planes[2] = bot_plane;
    planes[3] = top_plane;  planes[4] = near_plane;  planes[5] = far_plane;
    return planes;
}

void Renderer::update_projection_matrix(float fov) {
    int height, width;
    SDL_GL_GetDrawableSize(this->window, &width, &height);
    float aspect = (float) width / (float) height;
    this->projection_matrix = gen_projection_matrix(1, -10, fov, aspect);
    glViewport(0, 0, width, height);
    /// Update all shader programs projection matrices to the new one
    for (auto shader_program : shaders) {
        glUseProgram(shader_program.second.gl_program);
        GLuint projection = glGetUniformLocation(shader_program.second.gl_program, "projection");
        glUniformMatrix4fv(projection, 1, GL_FALSE, projection_matrix.data());
    }
}

uint64_t Renderer::add_to_batch(RenderComponent component, Mesh mesh) {
    for (auto &batch : graphics_batches) {
        if (batch.hash_id == component.entity->hash_id) {
            batch.components.push_back(component);
            return render_components_id++;
        }
    }

    GraphicsBatch batch{component.entity->hash_id};
    batch.mesh = mesh;
    batch.gl_shader_program = shaders.at(ShaderType::STANDARD_SHADER).gl_program;
    glUseProgram(batch.gl_shader_program);

    glGenVertexArrays(1, (GLuint *) &batch.gl_VAO);
    glBindVertexArray(batch.gl_VAO);
    batch.gl_camera_view = glGetUniformLocation(batch.gl_shader_program, "camera_view");

    GLuint gl_VBO;
    glGenBuffers(1, &gl_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, gl_VBO);
    auto vertices = batch.mesh.to_floats();
    glBufferData(GL_ARRAY_BUFFER, batch.mesh.byte_size_of_vertices(), vertices.data(), GL_STATIC_DRAW);

    // Then set our vertex attributes pointers, only doable AFTER linking
    GLuint positionAttrib = glGetAttribLocation(batch.gl_shader_program, "position");
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *)offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(positionAttrib);

    GLuint colorAttrib = glGetAttribLocation(batch.gl_shader_program, "vColor");
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>), (const void *)offsetof(Vertex<float>, color));
    glEnableVertexAttribArray(colorAttrib);

    // Buffer for all the model matrices
    glGenBuffers(1, (GLuint *) &batch.gl_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_models_buffer_object);

    GLuint modelAttrib = glGetAttribLocation(batch.gl_shader_program, "model");
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(modelAttrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>), (const void *) (sizeof(float) * i * 4));
        glEnableVertexAttribArray(modelAttrib + i);
        glVertexAttribDivisor(modelAttrib + i, 1);
    }

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.mesh.byte_size_of_indices(), batch.mesh.indices.data(), GL_STATIC_DRAW);

    GLuint projection = glGetUniformLocation(batch.gl_shader_program, "projection");
    glUniformMatrix4fv(projection, 1, GL_FALSE, projection_matrix.data());

    batch.components.push_back(component);
    graphics_batches.push_back(batch);

    return render_components_id++;
}

Mesh Renderer::load_mesh_from_file(std::string filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    auto success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str());
    if (!success) { SDL_Log("Failed loading mesh %s: %s", filepath.c_str(), err.c_str()); return Mesh(); }

    Mesh mesh{};
    for (auto shape : shapes) { // Shapes
        size_t index_offset = 0;
        for (auto face : shape.mesh.num_face_vertices) { // Faces (polygon)
            for (auto v = 0; v < face; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                // TODO: Take out normals and texcoords
                mesh.vertices.push_back(Vertex<float>{Vec3<float>{vx, vy, vz}});
                mesh.indices.push_back(3 * idx.vertex_index);
            }
            index_offset += face;
        }
    }

    return mesh;
}

void Renderer::remove_from_batch(RenderComponent component) {
    // TODO: Implement
}

void Renderer::update_render_component(RenderComponent component) {
    // TODO: This feels like a bad design, gotta rework it.
    for (auto &batch : graphics_batches) {
        if (batch.hash_id == component.entity->hash_id) {
            for (auto &old_component : batch.components) {
                if (component.id == old_component.id) {
                    old_component = component;
                }
            }
        }
    }
}


