
layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    in vec3 gsNormal;
    in vec2 gsTextureCoord;
    in vec3 gsPosition;
} gs_in[];

uniform mat4 ortho;
uniform uint voxel_grid_dimension;

out vec3 fNormal;   
out vec3 fPosition; // World space position
out vec2 fTextureCoord;

void main() {
    const vec3 x = vec3(1.0, 0.0, 0.0);
    const vec3 y = vec3(0.0, 1.0, 0.0);
    const vec3 z = vec3(0.0, 0.0, 1.0);

    const vec3 normal = normalize(cross(gs_in[1].gsPosition - gs_in[0].gsPosition, gs_in[2].gsPosition - gs_in[0].gsPosition));

    // Find the dominant axis of the triangle
    float max = abs(dot(normal, x));
    uint dominant_axis_projected = 0;
    
    if (max < abs(dot(normal, y))) {
        max = abs(dot(normal, y));
        dominant_axis_projected = 1;
    }  
    if (max < abs(dot(normal, z))) {
        dominant_axis_projected = 2;
    }

    // Conservative Rasterization setup
    vec4 gs_out[3];
    for (uint i = 0; i < 3; i++) {
      gs_out[i] = ortho * gl_in[i].gl_Position;

      if (dominant_axis_projected == 0) {
        gs_out[i].xyz = gs_out[i].zyx;
      } else if (dominant_axis_projected == 1) {
        gs_out[i].xyz = gs_out[i].xzy;
      }
    }

    // Enlarge the triangle with one texel size
    vec2 side0N = normalize(gs_out[1].xy - gs_out[0].xy);
    vec2 side1N = normalize(gs_out[2].xy - gs_out[1].xy);
    vec2 side2N = normalize(gs_out[0].xy - gs_out[2].xy);
    const float texel_size = 1.0 / float(voxel_grid_dimension);
    gs_out[0].xy += normalize(-side0N + side2N) * texel_size;
    gs_out[1].xy += normalize(side0N - side1N)  * texel_size;
    gs_out[2].xy += normalize(side1N - side2N)  * texel_size;

    // Emit the enlarged vertices
    for (uint i = 0; i < 3; i++) {
        gl_Position = gs_out[i];
        fNormal = gs_in[i].gsNormal;
        fPosition = gs_in[i].gsPosition;
        fTextureCoord = gs_in[i].gsTextureCoord;
        EmitVertex();
    }

    EndPrimitive();
}
