
layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    in vec3 gsNormal;
    in vec2 gsTextureCoord;
    in vec3 gsPosition;
} gs_in[];

uniform mat4 ortho_x;
uniform mat4 ortho_y;
uniform mat4 ortho_z;

out vec3 fNormal;   
out vec3 fPosition; // World space position
out vec2 fTextureCoord;
flat out uint dominant_axis_projected; // 0 = x, 1 = y, 2 = z

void main() {
    const vec3 x = vec3(1.0, 0.0, 0.0);
    const vec3 y = vec3(0.0, 1.0, 0.0);
    const vec3 z = vec3(0.0, 0.0, 1.0);

    const vec3 normal = normalize(cross(gs_in[1].gsPosition - gs_in[0].gsPosition, gs_in[2].gsPosition - gs_in[0].gsPosition));

    // Find the dominant axis of the triangle
    vec3 dominant_axis = x;
    mat4 ortho = ortho_x;
    float max = abs(dot(normal, x));
    dominant_axis_projected = 0;
    
    if (max < abs(dot(normal, y))) {
        dominant_axis = y;
        ortho = ortho_y;
        dominant_axis_projected = 1;
    }  
    if (max < abs(dot(normal, z))) {
        dominant_axis = z;
        ortho = ortho_z;
        dominant_axis_projected = 2;
    }

    // TODO: Conservative rasterization?

    // Orthographic projection along the dominant axis
    for (uint i = 0; i < 3; i++) {
        gl_Position = ortho * gl_in[i].gl_Position;
        fNormal = gs_in[i].gsNormal;
        fPosition = gl_in[i].gl_Position.xyz;
        fTextureCoord = gs_in[i].gsTextureCoord;
        EmitVertex();
    }

    EndPrimitive();
}