in vec3 position;
in vec2 texcoord;
#if defined(FLAG_CUBE_MAP_TEXTURE) || defined(FLAG_2D_TEXTURE)
in int diffuse_texture_idx;
#endif

// Model
in mat4 model;

// View or a.k.a camera matrix
uniform mat4 camera_view;

// Projection or a.k.a perspective matrix
uniform mat4 projection;

// All output variables gets interpolated 
out vec2 fTexcoord;
out vec4 fPosition;
out vec4 fNonModelPos;
#if defined(FLAG_CUBE_MAP_TEXTURE) || defined(FLAG_2D_TEXTURE)
flat out int fDiffuse_texture_idx;
#endif

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0);

  fTexcoord = texcoord;
  fPosition = model * vec4(position, 1.0);
  fNonModelPos = vec4(position, 1.0);
#if defined(FLAG_CUBE_MAP_TEXTURE) || defined(FLAG_2D_TEXTURE)
  fDiffuse_texture_idx = diffuse_texture_idx;
#endif
}
