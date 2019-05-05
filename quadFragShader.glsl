#version 440

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D blurredTexture;

void main()
{
    FragColor = texture(blurredTexture, TexCoords);
}