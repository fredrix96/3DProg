#version 440
in vec2 tex_FRAG_in;
in vec3 position_FRAG_in;
in vec3 normals_FRAG_in;
in vec3 tangent_FRAG_in;

layout (location = 0) out vec3 finalPos;
layout (location = 1) out vec3 finalNorm;
layout (location = 2) out vec3 finalMat;

uniform sampler2D Texture;
uniform sampler2D NormalMap;

vec3 calcNormalMap() { //this is for the normalmap
	vec3 normal = normalize(normals_FRAG_in);
	vec3 tangent = normalize(tangent_FRAG_in);

	//the bitangent should be perpendicular to the tangent and normal
	vec3 bitangent = cross(tangent, normal);

	vec3 normalMap = texture(NormalMap, tex_FRAG_in).xyz;

	//the normal map is stored as a color [0-1]. We transform it back to [0 to -1] and [1 to 1]
	normalMap = 2.0f * normalMap - vec3(1.0f);

	//the normal is now in tangentspace, we will transform it back to worldspace
	mat3 TBN = mat3(tangent, bitangent, normal);
	vec3 newNormal = normalize(TBN * normalMap);
	return newNormal;
}

void main () {
	finalPos = position_FRAG_in;
	finalNorm = calcNormalMap();
	finalNorm = normalize(finalNorm);
	finalMat = texture(Texture, tex_FRAG_in).rgb;
}