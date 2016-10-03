#include "render.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <cmath>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <map>
#include <fstream>
#include <array>

#include "../math/vector.h"
#include "primitives.h"
#include "../world/world.h"
#include "camera.h"

#include "graphicsbatch.h"
#include "rendercomponent.h"
#include "../nodes/entity.h"
#include "shader.h"

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
Mat4<float> Render::FPSViewRH(Vec3<float> eye, float pitch, float yaw) {
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

Render::Render(): skybox(Cube()), DRAW_DISTANCE(200), projection_matrix(Mat4<float>()), state{}, graphics_batches{}, shaders{} {
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

    /** Skybox **/
    skybox.scale = 500.0f;

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

    /// Skybox vertex bindings
    glGenVertexArrays(1, (GLuint *) &gl_skybox_VAO);
    glBindVertexArray(gl_skybox_VAO);

    this->gl_skybox_model  = glGetUniformLocation(shaders.at(ShaderType::SKYBOX_SHADER).gl_program, "model");
    this->gl_skybox_camera = glGetUniformLocation(shaders.at(ShaderType::SKYBOX_SHADER).gl_program, "camera");

    GLuint skybox_VBO;
    glGenBuffers(1, &skybox_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_VBO);
    auto sky_vector = skybox.to_floats();
    glBufferData(GL_ARRAY_BUFFER, skybox.byte_size_of_vertices(), sky_vector.data(), GL_STATIC_DRAW);

    // Then set our vertex attributes pointers, only doable AFTER linking
    auto sky_positionAttrib = glGetAttribLocation(shaders.at(ShaderType::SKYBOX_SHADER).gl_program, "position");
    glVertexAttribPointer(sky_positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>),
                          (const void *)offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(sky_positionAttrib);

    GLuint sky_EBO;
    glGenBuffers(1, &sky_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sky_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, skybox.byte_size_of_indices(), skybox.indices.data(), GL_STATIC_DRAW);

    /** Cube **/
    // Bind Vertex Array Object - OpenGL core REQUIRES a VERTEX ARRAY OBJECT in
    // order to render ANYTHING.
    // An object that stores vertex array bindings
    glGenVertexArrays(1, (GLuint *) &gl_VAO);
    glBindVertexArray(gl_VAO); // make the VAO the active one ..

    // Upload only one cube's quads since the gl_model is the same for all cubes
    Cube cube = Cube();

    GLuint gl_VBO;
    glGenBuffers(1, &gl_VBO);              // generate a 'name for the object'
    glBindBuffer(GL_ARRAY_BUFFER, gl_VBO); // bind/make VBO the active object
    glBufferData(GL_ARRAY_BUFFER, cube.byte_size_of_vertices(), NULL,
                 GL_STATIC_DRAW); // reserve a large buffer for cube quads

    auto vector = cube.to_floats();
    glBufferSubData(GL_ARRAY_BUFFER, 0, cube.byte_size_of_vertices(), vector.data());

    // Then set our vertex attributes pointers, only doable AFTER linking
    GLuint positionAttrib = glGetAttribLocation(shaders.at(ShaderType::STANDARD_SHADER).gl_program, "position");
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>),
                          (const void *)offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(positionAttrib);

    GLuint colorAttrib = glGetAttribLocation(shaders.at(ShaderType::STANDARD_SHADER).gl_program, "vColor");
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>),
                          (const void *)offsetof(Vertex<float>, color));
    glEnableVertexAttribArray(colorAttrib);

    // Buffer for all the model matrices
    glGenBuffers(1, (GLuint *) &gl_modelsBO);
    glBindBuffer(GL_ARRAY_BUFFER, gl_modelsBO);

    GLuint modelAttrib = glGetAttribLocation(shaders.at(ShaderType::STANDARD_SHADER).gl_program, "model");
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(modelAttrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>),
                              (const void *) (sizeof(float) * i * 4));
        glEnableVertexAttribArray(modelAttrib + i);
        glVertexAttribDivisor(modelAttrib + i, 1);
    }

    // View / camera space
    this->gl_camera_view = glGetUniformLocation(shaders.at(ShaderType::STANDARD_SHADER).gl_program, "camera_view");

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube.byte_size_of_indices(), cube.indices.data(), GL_STATIC_DRAW);

    // Camera
    auto position  = Vec3<float>{0.0f, 0.0f, 0.0f};  // cam position
    auto direction = Vec3<float>{0.0f, 0.0f, -1.0f}; // position of where the cam is looking
    auto world_up  = Vec3<float>{0.0, 1.0f, 0.0f};   // world up
    this->camera = std::make_shared<Camera>(position, direction, world_up);
}

bool Render::point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes) {
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

void Render::render_world(const World *world) {
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

        std::vector<Mat4<float>> buffer{};
        for (auto component : batch.components) {
            // glBindTexture(GL_TEXTURE_CUBE_MAP, textures[component.graphics_state.texture]);
            glBindTexture(GL_TEXTURE_CUBE_MAP, textures[Texture::SKYBOX]);

            Mat4<float> model{};
            model = model.translate(camera->position + camera->direction * 2);
            model = model.transpose();
            buffer.push_back(model);
        }
        glBindBuffer(GL_ARRAY_BUFFER, batch.gl_models_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(Mat4<float>), buffer.data(), GL_DYNAMIC_DRAW);
        glDrawElementsInstanced(GL_TRIANGLES, batch.mesh.indices.size(), GL_UNSIGNED_INT, 0, buffer.size());
    }

    /** Render skybox **/
    glUseProgram((GLuint) shaders.at(ShaderType::SKYBOX_SHADER).gl_program);
    glBindVertexArray(gl_skybox_VAO);
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_FRONT);   // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise
    glBindTexture(GL_TEXTURE_CUBE_MAP, textures[Texture::SKYBOX]);

    glUniformMatrix4fv(gl_skybox_camera, 1, GL_FALSE, camera_view.data());
    Mat4<float> model{};
    model = model.translate(camera->position);
    model = model.scale(skybox.scale);
    glUniformMatrix4fv(gl_skybox_model, 1, GL_TRUE, model.data());
    glDrawElements(GL_TRIANGLES, skybox.indices.size(), GL_UNSIGNED_INT, 0);

    /** Render cubes **/
    glUseProgram((GLuint) shaders.at(ShaderType::STANDARD_SHADER).gl_program);
    glBindVertexArray(gl_VAO);
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK);    // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise
    glUniformMatrix4fv(gl_camera_view, 1, GL_FALSE, camera_view.data());

    std::vector<Mat4<float>> buffer{};
    for (int j = 0; j < world->chunks.size(); j++) {
        auto chunk = &world->chunks[j];

        for (int i = 0; i < chunk->numCubes; i++) {
            auto cube = &chunk->blocks[i];

            // Frustrum cullling
            if (point_inside_frustrum(cube->position, planes)) { continue; }

            // Draw distance
            auto camera_to_cube = camera->position - cube->position;
            if (camera_to_cube.length() >= DRAW_DISTANCE) { continue; }

            glBindTexture(GL_TEXTURE_CUBE_MAP, textures[cube->texture]);
            // Model - transform_z * transform_y * transform_x * transform_translation * transform_scaling
            Mat4<float> model{};
            model = model.scale(cube->scale);
            model = model.rotate_x(cube->theta_x);
            model = model.rotate_y(cube->theta_y);
            model = model.rotate_z(cube->theta_z);
            model = model.translate(cube->position);
            model = model.transpose();
            buffer.push_back(model);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, gl_modelsBO);
    glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(Mat4<float>), buffer.data(), GL_DYNAMIC_DRAW);
    glDrawElementsInstanced(GL_TRIANGLES, skybox.indices.size(), GL_UNSIGNED_INT, 0, buffer.size());

    /// Update render state
    state.entities = buffer.size();
}

Render::~Render() {
    // TODO: Clear up all the GraphicsObjects
}

/// Returns the planes from the frustrum matrix in order; {left, right, bottom, top, near, far}
/// See: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
std::array<Plane<float>, 6> Render::extract_planes(Mat4<float> mat) {
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

void Render::update_projection_matrix(float fov) {
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

void Render::add_to_batch(RenderComponent component) {
    for (auto batch : graphics_batches) {
        if (batch.hash_id == component.entity->hash_id) {
            batch.components.push_back(component);
            return;
        }
    }

    GraphicsBatch batch{component.entity->hash_id};
    batch.mesh = Cube(); /// Default to mesh of a cube ...
    batch.gl_shader_program = shaders.at(ShaderType::STANDARD_SHADER).gl_program;
    glUseProgram(batch.gl_shader_program);
    // glEnable(GL_CULL_FACE); // cull face
    // glCullFace(GL_BACK);    // cull back face
    // glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise

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
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>),
                          (const void *)offsetof(Vertex<float>, position));
    glEnableVertexAttribArray(positionAttrib);

    GLuint colorAttrib = glGetAttribLocation(batch.gl_shader_program, "vColor");
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex<float>),
                          (const void *)offsetof(Vertex<float>, color));
    glEnableVertexAttribArray(colorAttrib);

    // Buffer for all the model matrices
    glGenBuffers(1, (GLuint *) &batch.gl_models_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, batch.gl_models_buffer_object);

    GLuint modelAttrib = glGetAttribLocation(batch.gl_shader_program, "model");
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(modelAttrib + i, 4, GL_FLOAT, GL_FALSE, sizeof(Mat4<float>),
                              (const void *) (sizeof(float) * i * 4));
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
}

void Render::remove_from_batch(RenderComponent component) {
    // TODO: Implement
}


