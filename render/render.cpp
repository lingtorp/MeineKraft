#include <fstream>
#include <iostream>
#include "render.h"

typedef enum { png, jpg } FileFormat;

// Texture loading order; right, left, top, bottom, back, front
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

// Camera combined rotation matrix (y, x) & translation matrix
Mat4<GLfloat> Render::FPSViewRH(Vec3 eye, float pitch, float yaw) {
    float rad = M_PI / 180;
    float cosPitch = cos(pitch * rad);
    float sinPitch = sin(pitch * rad);
    float cosYaw = cos(yaw * rad);
    float sinYaw = sin(yaw * rad);
    Vec3 xaxis = {cosYaw, 0, -sinYaw};
    Vec3 yaxis = {sinYaw * sinPitch, cosPitch, cosYaw * sinPitch};
    Vec3 zaxis = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};
    Mat4<GLfloat> matrix;
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
    matrix[3][0] = -dot(xaxis, eye);
    matrix[3][1] = -dot(yaxis, eye);
    matrix[3][2] = -dot(zaxis, eye); // GLM says no minus , other's say minus
    matrix[3][3] = 1.0f;
    return matrix;
}

Mat4<GLfloat> perspective_matrix(float z_near, float z_far, float fov,
                            float aspect) {
    float rad = M_PI / 180;
    float tanHalf = tan(fov * rad / 2);
    float a = 1 / (tanHalf * aspect);
    float b = 1 / tanHalf;
    float c = -(z_far + z_near) / (z_far - z_near);
    float d = -(2 * z_far * z_near) / (z_far - z_near);
    Mat4<GLfloat> matrix;
    matrix[0] = {a, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, b, 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, c, d};
    matrix[3] = {0.0f, 0.0f, -1.0f, 0.0f};
    return matrix;
}

Mat4<GLfloat> Render::scaling_matrix(float scale) {
    Mat4<GLfloat> matrix;
    matrix[0] = {scale, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, scale, 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, scale, 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Mat4<GLfloat> Render::translation_matrix(float x, float y, float z) {
    Mat4<GLfloat> matrix;
    matrix[0] = {1.0f, 0.0f, 0.0f, x};
    matrix[1] = {0.0f, 1.0f, 0.0f, y};
    matrix[2] = {0.0f, 0.0f, 1.0f, z};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

// Assumes the file exists and will seg. fault otherwise.
const std::string Render::load_shader_source(std::string filename) {
    std::ifstream ifs(filename);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                              (std::istreambuf_iterator<char>()));
}

// Transformation matrix for the y-plane
Mat4<GLfloat> Render::transformation_matrix_x(float theta) {
    float x = M_PI / 180;
    Mat4<GLfloat> matrix;
    matrix[0] = {1.0f, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, cosf(theta * x), -sinf(theta * x), 0.0f};
    matrix[2] = {0.0f, sinf(theta * x), cosf(theta * x), 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Mat4<GLfloat> Render::transformation_matrix_y(float theta) {
    float x = M_PI / 180;
    Mat4<GLfloat> matrix;
    matrix[0] = {cosf(theta * x), 0.0f, sinf(theta * x), 0.0f};
    matrix[1] = {0.0f, 1.0f, 0.0f, 0.0f};
    matrix[2] = {-sinf(theta * x), 0.0f, cosf(theta * x), 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Mat4<GLfloat> Render::transformation_matrix_z(float theta) {
    float x = M_PI / 180;
    Mat4<GLfloat> matrix;
    matrix[0] = {cosf(theta * x), -sinf(theta * x), 0.0f, 0.0f};
    matrix[1] = {sinf(theta * x), cosf(theta * x), 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, 1.0f, 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Render::Render(SDL_Window *window): skybox(Cube()) {
    glewExperimental = (GLboolean) true;
    glewInit();

    this->window = window;

    // Load all block & skybox textures
    this->textures = (GLuint *) calloc(4, sizeof(GLuint));

    std::vector<std::string> cube_faces = {"res/blocks/grass/bottom.jpg", "res/blocks/grass/bottom.jpg",
                                           "res/blocks/grass/bottom.jpg", "res/blocks/grass/bottom.jpg",
                                           "res/blocks/grass/bottom.jpg", "res/blocks/grass/bottom.jpg"};
    this->textures[GRASS] = load_cube_map(cube_faces, jpg);

    std::vector<std::string> skybox_faces = {"res/sky/right.jpg", "res/sky/left.jpg",
                                             "res/sky/top.jpg",   "res/sky/bottom.jpg",
                                             "res/sky/back.jpg",  "res/sky/front.jpg"};
    this->textures[SKYBOX] = load_cube_map(skybox_faces, jpg);

    // Upload only one cube's quads since the model is the same for all cubes
    Cube cube = Cube();

    GLuint VBO;                         // VBO - vertex buffer object
    glGenBuffers(1, &VBO);              // generate a 'name for the object'
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind/make VBO the active object
    glBufferData(GL_ARRAY_BUFFER, cube.byte_size_of_vertices(), NULL,
                 GL_STATIC_DRAW); // reserve a large buffer for cube quads
    this->VBO = VBO;

    auto vector = cube.to_floats();
    glBufferSubData(GL_ARRAY_BUFFER, 0, cube.byte_size_of_vertices(), vector.data());

    auto vertexShaderSource =
            load_shader_source("shaders/block/vertex-shader.glsl").c_str();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint vertexShaderStatus;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexShaderStatus);

    auto fragmentShaderSource =
            load_shader_source("shaders/block/fragment-shader.glsl").c_str();
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    GLint fragmentShaderStatus;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentShaderStatus);

    printf("Fragment: %i, Vertex: %i\n", fragmentShaderStatus,
           vertexShaderStatus);
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    printf("%s\n", buffer);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    this->shader_program = shaderProgram;

    // Bind Vertex Array Object - OpenGL core REQUIRES a VERTEX ARRAY OBJECT in
    // order to render ANYTHING.
    // An object that stores vertex array bindings
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); // make the VAO the active one ..
    this->VAO = VAO;

    // Then set our vertex attributes pointers, only doable AFTER linking
    GLuint positionAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(positionAttrib);

    GLuint colorAttrib = glGetAttribLocation(shaderProgram, "vColor");
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(colorAttrib);

    this->transform_x = glGetUniformLocation(shaderProgram, "transform_x");
    this->transform_y = glGetUniformLocation(shaderProgram, "transform_y");
    this->transform_z = glGetUniformLocation(shaderProgram, "transform_z");

    // Model
    this->transform_translation =
            glGetUniformLocation(shaderProgram, "transform_translation");
    this->transform_scaling =
            glGetUniformLocation(shaderProgram, "transform_scaling");

    // View / camera space
    this->camera_view = glGetUniformLocation(shaderProgram, "camera_view");

    // Projection
    GLuint transform_perspective =
            glGetUniformLocation(shaderProgram, "transform_perspective");
    int height, width;
    SDL_GetWindowSize(this->window, &width, &height);
    float aspect = width / height;
    auto transMat_perspective = perspective_matrix(1, -20, 60, aspect);
    glUniformMatrix4fv(transform_perspective, 1, GL_FALSE, transMat_perspective.data());

    GLuint indices[] = {
            // front
            0, 1, 2, 2, 3, 0,
            // right
            1, 5, 6, 6, 2, 1,
            // back
            7, 6, 5, 5, 4, 7,
            // left
            4, 0, 3, 3, 7, 4,
            // bot
            4, 5, 1, 1, 0, 4,
            // top
            3, 2, 6, 6, 7, 3};

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    /****** Skybox ******/
    Cube skybox = Cube();
    skybox.scale = 20.0f;
    this->skybox = skybox;

    // Camera
    Vec3 position  = {0.0f, 0.0f, 1.0f};  // cam position
    Vec3 direction = {0.0f, 0.0f, -1.0f}; // position of where the cam is looking
    Vec3 world_up  = {0.0, 1.0f, 0.0f};   // world up
    this->camera = std::make_shared<Camera>(position, direction, world_up);
}

void Render::render_world(World *world) {
    // camera view matrix
    auto transMat_camera_view = FPSViewRH(camera->position, camera->pitch, camera->yaw);

    // Draw
    glUseProgram(shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.3f, 0.7f, 1.0f);
    glUniformMatrix4fv(camera_view, 1, GL_FALSE, transMat_camera_view.data());

    /** Render skybox **/
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_FRONT);   // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise
    glBindTexture(GL_TEXTURE_CUBE_MAP, textures[SKYBOX]);

    // Model TODO: Make the skybox follow the player
    auto transMat_scaling = scaling_matrix(skybox.scale);
    glUniformMatrix4fv(transform_scaling, 1, GL_TRUE, transMat_scaling.data());

    auto transMat_translation = translation_matrix(
            skybox.position.x, skybox.position.y, skybox.position.z);
    glUniformMatrix4fv(transform_translation, 1, GL_TRUE, transMat_translation.data());

    auto transMat_x = transformation_matrix_x(0.0f);
    glUniformMatrix4fv(transform_x, 1, GL_TRUE, transMat_x.data());
    auto transMat_y = transformation_matrix_y(0.0f);
    glUniformMatrix4fv(transform_y, 1, GL_TRUE, transMat_y.data());
    auto transMat_z = transformation_matrix_z(0.0f);
    glUniformMatrix4fv(transform_z, 1, GL_TRUE, transMat_z.data());

    // GOTTA BIND THE VAO TO TELL OPENGL WHERE THE INDICES ARE FROM
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);

    /** Render cubes **/
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK);    // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise

    for (auto chunk : world->chunks) {
        for (auto cube : chunk.blocks) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, textures[cube.texture]);

            // Model
            transMat_scaling = scaling_matrix(cube.scale);
            glUniformMatrix4fv(transform_scaling, 1, GL_TRUE, transMat_scaling.data());

            transMat_translation =
                    translation_matrix(cube.position.x, cube.position.y, cube.position.z);
            glUniformMatrix4fv(transform_translation, 1, GL_TRUE,
                               transMat_translation.data());

            transMat_x = transformation_matrix_x(cube.theta_x);
            glUniformMatrix4fv(transform_x, 1, GL_TRUE, transMat_x.data());
            transMat_y = transformation_matrix_y(cube.theta_y);
            glUniformMatrix4fv(transform_y, 1, GL_TRUE, transMat_y.data());
            transMat_z = transformation_matrix_z(cube.theta_z);
            glUniformMatrix4fv(transform_z, 1, GL_TRUE, transMat_z.data());

            // GOTTA BIND THE VAO TO TELL OPENGL WHERE THE INDICES ARE FROM
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);
        }
    }
}

Render::~Render() {
    glDeleteVertexArrays(1, (const GLuint *) this->VAO);
    glDeleteBuffers(1, (const GLuint *) this->VBO);
}
