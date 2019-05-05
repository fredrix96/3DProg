#version 440

//ccw means that the tesselator will emit triangles in counter-clockwise order
//this really depends on how the models are generated in Blender or other engines
layout(triangles, equal_spacing, ccw) in;

in vec3 position_TES_in[];
in vec2 tex_TES_in[];
in vec3 normals_TES_in[];
in vec3 tangent_TES_in[];

out vec2 tex_GEO_in;
out vec3 normals_GEO_in;
out vec3 tangent_GEO_in;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2)
{
   	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
   	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main() {
    //interpolating the attributes of the output vertex using barycentric coordinates
    vec3 position_GEO_in = interpolate3D(position_TES_in[0], position_TES_in[1], position_TES_in[2]);
    tex_GEO_in = interpolate2D(tex_TES_in[0], tex_TES_in[1], tex_TES_in[2]);
    normals_GEO_in = interpolate3D(normals_TES_in[0], normals_TES_in[1], normals_TES_in[2]);
    normals_GEO_in = normalize(normals_GEO_in);
    tangent_GEO_in = interpolate3D(tangent_TES_in[0], tangent_TES_in[1], tangent_TES_in[2]);
    tangent_GEO_in = normalize(tangent_GEO_in);
    gl_Position = vec4(position_GEO_in, 1.0f);
}