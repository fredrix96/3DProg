#version 440
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_FRAG_in;

void main() {
	gl_Position = vec4(vertex_position, 1.0);
	tex_FRAG_in = tex_coord;
}