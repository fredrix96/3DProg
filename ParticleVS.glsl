#version 440 core

layout(location = 0) in vec3 vertices; //(BillboardPos)
layout(location = 1) in vec4 pos; // Position of the center of the particule and size of the square
layout(location = 2) in vec4 color; // Position of the center of the particule and size of the square

out vec2 UV;
out vec4 particleColor;

uniform vec3 camRight;
uniform vec3 camUp;
uniform mat4 view, proj; //(the position is in BillboardPos; the orientation depends on the camera)
mat4 VP = proj * view;

void main()
{
	float particleSize = pos.w;
	vec3 particleCenter = pos.xyz;
	
	vec3 vertexPos = particleCenter 
		+ camRight * vertices.x * particleSize
		+ camUp * vertices.y * particleSize;

	gl_Position = VP * vec4(vertexPos, 1.0f);

	UV = vertices.xy + vec2(0.5, 0.5);
	particleColor = color;
}

