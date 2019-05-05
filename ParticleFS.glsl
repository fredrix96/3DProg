#version 440 core

in vec2 UV;
in vec4 particleColor;

layout (location = 3) out vec4 color;

uniform sampler2D particleTexture;

void main(){
	color = texture(particleTexture, UV ) * particleColor;
    if(color.a==0.0) discard;
}