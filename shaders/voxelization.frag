
layout(RGBA8) uniform image3D voxel_data;

in vec3 fNormal;   
in vec3 fPosition; // World space position
in vec2 fTextureCoord;
flat in uint dominant_axis_projected;

struct PointLight { // NOTE: Defined in lighting.frag also
    vec4 position;  // (X, Y, Z, padding)
    vec4 intensity; // (R, G, B, padding)
};

layout(std140, binding = 4) buffer PointLightBlock {
    PointLight pointlights[];
};

out vec4 color;

// NOTE: Adding all lights here for the direct lighting is weird, might be doable with only the shadowmap information. Need to experiment with both approaches.
// TODO: fPosition is in world space, need to have it to [0, textureSize] in order to get the voxel position

// TODO: Implement a real attenuation formula
const float a = 0.5;
const float b = 0.5;
float attenuation(float distance) {
    return 1.0 / (1.0 + a * distance + b * distance * distance); 
}

/// Phong shading for now
vec3 diffuse_lighting(vec3 position, vec3 normal, PointLight light) {
    const vec3 direction = normalize(light.position.xyz - position);
    const float cos_theta = max(dot(normalize(normal), direction), 0.0);
    return cos_theta * attenuation(distance(position, light.position.xyz)) * light.intensity.rgb;
}

void main() {
    vec3 color = vec3(0.0);
    for (uint i = 0; i < pointlights.length(); i++) { color = diffuse_lighting(fPosition, fNormal, pointlights[i]); }

    const uint voxel_grid_size = imageSize(voxel_data).x; // Assuming x = y = z in grid size
    const vec3 voxel = gl_FragCoord.xyz * voxel_grid_size;
    ivec3 voxel_position;
    switch (dominant_axis_projected) {
      case 0: // x axis
      voxel_position = ivec3(voxel.z, voxel.y, voxel.x);
      break;
      case 1: // y axis
      voxel_position = ivec3(voxel.z, voxel.x, voxel.y);
      break;
      case 2: // z axis
      voxel_position = ivec3(voxel.x, voxel.y, voxel.z);
      break;
    }

    imageStore(voxel_data, voxel_position, vec4(color, 1.0));
}