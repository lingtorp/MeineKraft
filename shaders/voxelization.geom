// #version 450 core

#define NUM_CLIPMAPS 4

uniform bool uConservative_rasterization_enabled;

layout(triangles, invocations = NUM_CLIPMAPS) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    in vec3 gsNormal;
    in vec2 gsTextureCoord;
    in vec3 gsPosition;
} gs_in[];

uniform mat4 uOrthos[NUM_CLIPMAPS];
uniform int uClipmap_sizes[NUM_CLIPMAPS];

out vec3 fNormal;   
out vec3 fPosition; // World space position
out vec2 fTextureCoord;
out vec4 fAABB;

vec2 vmax(const vec2 a, const vec2 b) {
  return vec2(max(a.x, b.x), max(a.y, b.y));
}

vec2 vmin(const vec2 a, const vec2 b) { 
  return vec2(min(a.x, b.x), min(a.y, b.y));
}

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
      gs_out[i] = uOrthos[gl_InvocationID] * gl_in[i].gl_Position;

      if (dominant_axis_projected == 0) {
        gs_out[i].xyz = gs_out[i].zyx;
      } else if (dominant_axis_projected == 1) {
        gs_out[i].xyz = gs_out[i].xzy;
      }
    }

    // Used in conservative rasterization to change winding order
    vec2 gs_in_gsTextureCoord[3] = {
      gs_in[0].gsTextureCoord,
      gs_in[1].gsTextureCoord,
      gs_in[2].gsTextureCoord
    };

    vec3 gs_in_gsPosition[3] = {
      gs_in[0].gsPosition,
      gs_in[1].gsPosition,
      gs_in[2].gsPosition
    };

    vec3 gs_in_gsNormal[3] = {
      gs_in[0].gsNormal,
      gs_in[1].gsNormal,
      gs_in[2].gsNormal
    };			

    if (uConservative_rasterization_enabled) {
      if (false) {
        // Enlarge the triangle with one texel size
        vec2 side0N = normalize(gs_out[1].xy - gs_out[0].xy);
        vec2 side1N = normalize(gs_out[2].xy - gs_out[1].xy);
        vec2 side2N = normalize(gs_out[0].xy - gs_out[2].xy);
        const float texel_size = 1.4142135637309 / float(uClipmap_sizes[gl_InvocationID]);
        gs_out[0].xy += normalize(-side0N + side2N) * texel_size;
        gs_out[1].xy += normalize(side0N - side1N)  * texel_size;
        gs_out[2].xy += normalize(side1N - side2N)  * texel_size;
      } else {
         // Change winding, otherwise there are artifacts for the back faces.
        if (dot(normal, vec3(0.0, 0.0, 1.0)) < 0.0) {
			const vec4 gs_tmp = gs_out[2];

			gs_out[2] = gs_out[1];
			gs_in_gsTextureCoord[2] = gs_in[1].gsTextureCoord;
			gs_in_gsPosition[2] = gs_in[1].gsPosition;
			gs_in_gsNormal[2] = gs_in[1].gsNormal;

			gs_out[1] = gs_tmp;
			gs_in_gsTextureCoord[1] = gs_in[2].gsTextureCoord;
			gs_in_gsPosition[1] = gs_in[2].gsPosition;
			gs_in_gsNormal[1] = gs_in[2].gsNormal;
		 }

        // Enlarge the triangle
        const vec2 half_pixel = vec2(1.0 / float(uClipmap_sizes[gl_InvocationID]));

        // Calculate AABB of the triangle
        fAABB.xy = gs_out[0].xy;
        fAABB.zw = gs_out[0].xy;

        fAABB.xy = vmin(gs_out[1].xy, fAABB.xy);
        fAABB.zw = vmax(gs_out[1].xy, fAABB.zw);

        fAABB.xy = vmin(gs_out[2].xy, fAABB.xy);
        fAABB.zw = vmax(gs_out[2].xy, fAABB.zw);

        // Enlarge half-pixel
        fAABB.xy -= half_pixel;
        fAABB.zw += half_pixel;

        const float pl = 1.4142135637309 / uClipmap_sizes[gl_InvocationID];

        const vec3 e0 = vec3( gs_out[1].xy - gs_out[0].xy, 0 );
        const vec3 e1 = vec3( gs_out[2].xy - gs_out[1].xy, 0 );
        const vec3 e2 = vec3( gs_out[0].xy - gs_out[2].xy, 0 );
        const vec3 n0 = cross( e0, vec3(0,0,1) );
        const vec3 n1 = cross( e1, vec3(0,0,1) );
        const vec3 n2 = cross( e2, vec3(0,0,1) );

        // Dilate the triangle
        gs_out[0].xy += pl*( (e2.xy/dot(e2.xy,n0.xy)) + (e0.xy/dot(e0.xy,n2.xy)) );
        gs_out[1].xy += pl*( (e0.xy/dot(e0.xy,n1.xy)) + (e1.xy/dot(e1.xy,n0.xy)) );
        gs_out[2].xy += pl*( (e1.xy/dot(e1.xy,n2.xy)) + (e2.xy/dot(e2.xy,n1.xy)) );
      }
    }

    // Emit the vertices
    for (uint i = 0; i < 3; i++) {
        gl_Position = gs_out[i];
        fNormal = gs_in_gsNormal[i];
        fPosition = gs_in_gsPosition[i];
        fTextureCoord = gs_in_gsTextureCoord[i];
        gl_ViewportIndex = gl_InvocationID;
        EmitVertex();
    }

    EndPrimitive();
}
