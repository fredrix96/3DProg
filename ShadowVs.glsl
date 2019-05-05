#version 440

layout (location = 0) in vec3 vertex_position_shadow;

uniform mat4 lightSpaceMatrix;
uniform mat4 world;

void main()
{
    gl_Position = lightSpaceMatrix * world * vec4(vertex_position_shadow, 1.0f);
}  