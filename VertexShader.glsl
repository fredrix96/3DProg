#version 440
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal_faces;
layout(location = 3) in vec3 tangent_coord;

out vec3 position_TCS_in;
out vec2 tex_TCS_in;
out vec3 normals_TCS_in;
out vec3 tangent_TCS_in;

void main() {
	tex_TCS_in = tex_coord;
	normals_TCS_in = normal_faces;
	position_TCS_in = vertex_position;
	tangent_TCS_in = tangent_coord;
}