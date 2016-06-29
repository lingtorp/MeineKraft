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

Vec3 Render::camera_move_forward(const Camera *cam) {
    Vec3 movement =
            vec_scalar_multiplication(cam->direction, cam->movement_speed);
    return vec_addition(cam->position, movement);
}

Vec3 Render::camera_move_backward(const Camera *cam) {
    Vec3 movement =
            vec_scalar_multiplication(cam->direction, cam->movement_speed);
    return vec_subtraction(cam->position, movement);
}

Vec3 Render::camera_move_right(const Camera *cam) {
    Vec3 movement = normalize(cross(cam->direction, cam->up));
    return vec_addition(cam->position,
                        vec_scalar_multiplication(movement, cam->movement_speed));
}

Vec3 Render::camera_move_left(const Camera *cam) {
    Vec3 movement = normalize(cross(cam->direction, cam->up));
    return vec_subtraction(
            cam->position, vec_scalar_multiplication(movement, cam->movement_speed));
}

Vec3 Render::camera_direction(const Camera *cam) {
    float rad = M_PI / 180;
    float pitch = cam->pitch;
    Vec3 result;
    result.x = -sin(cam->yaw * rad) * cos(pitch * rad);
    result.y = sin(pitch * rad);
    result.z = -cos(cam->yaw * rad) * cos(pitch * rad);
    return normalize(result);
}

// Camera combined rotation matrix (y, x) & translation matrix
GLfloat *Render::FPSViewRH(Vec3 eye, float pitch, float yaw) {
    float rad = M_PI / 180;
    float cosPitch = cos(pitch * rad);
    float sinPitch = sin(pitch * rad);
    float cosYaw = cos(yaw * rad);
    float sinYaw = sin(yaw * rad);
    Vec3 xaxis = {cosYaw, 0, -sinYaw};
    Vec3 yaxis = {sinYaw * sinPitch, cosPitch, cosYaw * sinPitch};
    Vec3 zaxis = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};
    GLfloat tempMatrix[4][4];
    tempMatrix[0][0] = xaxis.x;
    tempMatrix[0][1] = yaxis.x;
    tempMatrix[0][2] = zaxis.x;
    tempMatrix[0][3] = 0.0f;
    tempMatrix[1][0] = xaxis.y;
    tempMatrix[1][1] = yaxis.y;
    tempMatrix[1][2] = zaxis.y;
    tempMatrix[1][3] = 0.0f;
    tempMatrix[2][0] = xaxis.z;
    tempMatrix[2][1] = yaxis.z;
    tempMatrix[2][2] = zaxis.z;
    tempMatrix[2][3] = 0.0f;
    tempMatrix[3][0] = -dot(xaxis, eye);
    tempMatrix[3][1] = -dot(yaxis, eye);
    tempMatrix[3][2] = -dot(zaxis, eye); // GLM says no minus , other's say minus
    tempMatrix[3][3] = 1.0f;
    GLfloat *matrix = (GLfloat *) calloc(1, sizeof(tempMatrix));
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

GLfloat *perspective_matrix(float z_near, float z_far, float fov,
                            float aspect) {
    float rad = M_PI / 180;
    float tanHalf = tan(fov * rad / 2);
    float a = 1 / (tanHalf * aspect);
    float b = 1 / tanHalf;
    float c = -(z_far + z_near) / (z_far - z_near);
    float d = -(2 * z_far * z_near) / (z_far - z_near);
    GLfloat tempMatrix[4][4] = {{a, 0.0f, 0.0f, 0.0f},
                                {0.0f, b, 0.0f, 0.0f},
                                {0.0f, 0.0f, c, d},
                                {0.0f, 0.0f, -1.0f, 0.0f}};
    GLfloat *matrix = (GLfloat *) calloc(1, sizeof(tempMatrix));
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

GLfloat *Render::scaling_matrix(float scale) {
    GLfloat tempMatrix[4][4] = {{scale, 0.0f, 0.0f, 0.0f},
                                {0.0f, scale, 0.0f, 0.0f},
                                {0.0f, 0.0f, scale, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};
    GLfloat *matrix = (GLfloat *)  calloc(1, sizeof(tempMatrix));
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

GLfloat *Render::translation_matrix(float x, float y, float z) {
    GLfloat tempMatrix[4][4] = {{1.0f, 0.0f, 0.0f, x},
                                {0.0f, 1.0f, 0.0f, y},
                                {0.0f, 0.0f, 1.0f, z},
                                {0.0f, 0.0f, 0.0f, 1.0f}};
    GLfloat *matrix = (GLfloat *)  calloc(1, sizeof(tempMatrix));
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

// Converts a quad to vertices
// 1 quad = 2 triangles => 4 vertices = 4 points & 4 colors
GLfloat *Render::quad_to_floats(Quad *quad) {
    GLfloat *floats = (GLfloat *) calloc(1, 4 * sizeof(Vertex));
    int j = 0;
    for (size_t i = 0; i < 4 * 9; i += 9) {
        Vertex vertex = quad->vertices[j];
        floats[i] = vertex.position.x;
        floats[i + 1] = vertex.position.y;
        floats[i + 2] = vertex.position.z;
        floats[i + 3] = vertex.color.r;
        floats[i + 4] = vertex.color.g;
        floats[i + 5] = vertex.color.b;
        floats[i + 6] = vertex.color.a;
        floats[i + 7] = vertex.texCoord.x;
        floats[i + 8] = vertex.texCoord.y;
        j++;
    }
    return floats;
}

// Assumes the file exists and will seg. fault otherwise.
const GLchar *Render::load_shader_source(std::string filename) {
    assert(filename.c_str() != NULL);
    FILE *file = std::fopen(filename.c_str(), "r"); // open
    fseek(file, 0L, SEEK_END);                      // find the end
    size_t size = ftell(file);                      // get the size in bytes
    GLchar *shaderSource = (GLchar *) calloc(1, size);  // allocate enough bytes
    rewind(file);                                   // go back to file beginning
    fread(shaderSource, size, sizeof(char), file);  // read each char into our block
    fclose(file);                                   // close the stream
    return shaderSource;
}

// Transformation matrix for the y-plane
GLfloat *Render::transformation_matrix_x(float theta) {
    float x = M_PI / 180;
    GLfloat *matrix = (GLfloat *) calloc(1, sizeof(GLfloat) * 4 * 4);
    GLfloat tempMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                {0.0f, cosf(theta * x), -sinf(theta * x), 0.0f},
                                {0.0f, sinf(theta * x), cosf(theta * x), 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

GLfloat *Render::transformation_matrix_y(float theta) {
    float x = M_PI / 180;
    GLfloat *matrix = (GLfloat *) calloc(1, sizeof(GLfloat) * 4 * 4);
    GLfloat tempMatrix[4][4] = {{cosf(theta * x), 0.0f, sinf(theta * x), 0.0f},
                                {0.0f, 1.0f, 0.0f, 0.0f},
                                {-sinf(theta * x), 0.0f, cosf(theta * x), 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

GLfloat *Render::transformation_matrix_z(float theta) {
    float x = M_PI / 180;
    GLfloat *matrix = (GLfloat *) calloc(1, sizeof(GLfloat) * 4 * 4);
    GLfloat tempMatrix[4][4] = {{cosf(theta * x), -sinf(theta * x), 0.0f, 0.0f},
                                {sinf(theta * x), cosf(theta * x), 0.0f, 0.0f},
                                {0.0f, 0.0f, 1.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};
    memcpy(matrix, tempMatrix, sizeof(tempMatrix));
    return matrix;
}

Render::Render(SDL_Window *window) {
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

    GLuint VBO;                         // VBO - vertex buffer object
    glGenBuffers(1, &VBO);              // generate a 'name for the object'
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind/make VBO the active object
    glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * 2, NULL,
                 GL_STATIC_DRAW); // reserve a large buffer for cube quads
    this->VBO = VBO;

    // Upload only one cube's quads since the model is the same for all cubes
    Color4 colors[] = {new_color_clear(), new_color_clear(), new_color_clear(), new_color_clear()};
    Cube *cube = new_cube(colors);
    for (size_t i = 0; i < 2; i++) {
        Quad quad = cube->quads[i];
        GLfloat *data = quad_to_floats(&quad);
        glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(Quad), sizeof(Quad), data);
    }
    this->VBO = VBO;

    const GLchar *vertexShaderSource =
            load_shader_source("shaders/block/vertex-shader.glsl");
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint vertexShaderStatus;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexShaderStatus);

    const GLchar *fragmentShaderSource =
            load_shader_source("shaders/block/fragment-shader.glsl");
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
    GLfloat *transMat_perspective = perspective_matrix(1, -20, 60, aspect);
    glUniformMatrix4fv(transform_perspective, 1, GL_FALSE, transMat_perspective);

    GLuint indices[] = {// front
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
    Cube *skybox = new_cube(colors);
    skybox->scale = 80.0f;
    this->skybox = skybox;

    // Camera
    Camera *camera = (Camera *) calloc(1, sizeof(Camera));
    Vec3 eye = {0.0f, 0.0f, 1.0f};     // cam position
    Vec3 center = {0.0f, 0.0f, -1.0f}; // position of where the cam is looking
    Vec3 up = {0.0, 1.0f, 0.0f};       // world up
    camera->position = eye;
    camera->direction = center;
    camera->up = up;
    camera->movement_speed = 0.5f;
    this->camera = camera;
}

void Render::render_world(World *world) {
    Cube *skybox = this->skybox;
    GLuint VAO = this->VAO;
    GLuint camera_view = this->camera_view;
    GLuint transform_x = this->transform_x;
    GLuint transform_y = this->transform_y;
    GLuint transform_z = this->transform_z;
    GLuint transform_translation = this->transform_translation;
    GLuint transform_scaling = this->transform_scaling;
    Camera *camera = this->camera;

    GLfloat *transMat_x;
    GLfloat *transMat_y;
    GLfloat *transMat_z;
    GLfloat *transMat_scaling;
    GLfloat *transMat_translation;

    // camera view matrix
    GLfloat *transMat_camera_view = this->FPSViewRH(camera->position, camera->pitch, camera->yaw);

    // Draw
    glUseProgram(this->shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.3f, 0.7f, 1.0f);
    glUniformMatrix4fv(camera_view, 1, GL_FALSE, transMat_camera_view);

    /** Render skybox **/
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_FRONT);   // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->textures[SKYBOX]);

    // Model TODO: Make the skybox follow the player
    transMat_scaling = scaling_matrix(skybox->scale);
    glUniformMatrix4fv(transform_scaling, 1, GL_TRUE, transMat_scaling);

    transMat_translation = translation_matrix(
            skybox->position.x, skybox->position.y, skybox->position.z);
    glUniformMatrix4fv(transform_translation, 1, GL_TRUE, transMat_translation);

    transMat_x = transformation_matrix_x(0.0f);
    glUniformMatrix4fv(transform_x, 1, GL_TRUE, transMat_x);
    transMat_y = transformation_matrix_y(0.0f);
    glUniformMatrix4fv(transform_y, 1, GL_TRUE, transMat_y);
    transMat_z = transformation_matrix_z(0.0f);
    glUniformMatrix4fv(transform_z, 1, GL_TRUE, transMat_z);

    // GOTTA BIND THE VAO TO TELL OPENGL WHERE THE INDICES ARE FROM
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);

    /** Render cubes **/
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK);    // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise

    for (size_t j = 0; j < World::MAX_CHUNKS; j++) {
        Chunk *chunk = world->chunks[j];
        for (size_t i = 0; i < chunk->numCubes; i++) {
            Cube cube = chunk->blocks[i];

            // TODO: Blocks should be able to have different textures.
            glBindTexture(GL_TEXTURE_CUBE_MAP, this->textures[cube.texture]);

            // Model
            transMat_scaling = scaling_matrix(cube.scale);
            glUniformMatrix4fv(transform_scaling, 1, GL_TRUE, transMat_scaling);

            transMat_translation =
                    translation_matrix(cube.position.x, cube.position.y, cube.position.z);
            glUniformMatrix4fv(transform_translation, 1, GL_TRUE,
                               transMat_translation);

            transMat_x = transformation_matrix_x(cube.theta_x);
            glUniformMatrix4fv(transform_x, 1, GL_TRUE, transMat_x);
            transMat_y = transformation_matrix_y(cube.theta_y);
            glUniformMatrix4fv(transform_y, 1, GL_TRUE, transMat_y);
            transMat_z = transformation_matrix_z(cube.theta_z);
            glUniformMatrix4fv(transform_z, 1, GL_TRUE, transMat_z);

            // GOTTA BIND THE VAO TO TELL OPENGL WHERE THE INDICES ARE FROM
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);
        }
    }

    free(transMat_camera_view); // FIXME: This is wasteful
    free(transMat_x);
    free(transMat_y);
    free(transMat_z);
    free(transMat_scaling);
    free(transMat_translation);
}

Render::~Render() {
    glDeleteVertexArrays(1, (const GLuint *) this->VAO);
    glDeleteBuffers(1, (const GLuint *) this->VBO);
}
