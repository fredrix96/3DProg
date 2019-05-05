#version 440
layout (location = 0) in vec4 aPos;

uniform mat4 world, proj, view;
mat4 WVP = proj * view * world;

void main()
{             
    gl_Position = WVP * aPos;
}  