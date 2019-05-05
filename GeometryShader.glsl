#version 440
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2 tex_GEO_in[];
in vec3 normals_GEO_in[];
in vec3 tangent_GEO_in[];

out vec2 tex_FRAG_in;
out vec3 position_FRAG_in;
out vec3 normals_FRAG_in;
out vec3 tangent_FRAG_in;

uniform mat4 proj, view, world;
uniform vec3 camPos;

void main() {

	mat4 WVP = proj * view * world;
	vec4 vertexPos;
	vec3 normal;
	vec3 camToVertex;
	float dotProdCull;
	
	for(int i = 0; i < gl_in.length(); i++) {
		//beräkna normal;
		normal = vec3(world * vec4(normals_GEO_in[i], 0.0f));
		//vektor från camera till vertex
		camToVertex = camPos.xyz - (world * gl_in[i].gl_Position).xyz;
		dotProdCull = dot(normal.xyz, camToVertex.xyz);

		if(dotProdCull > 0.0f)
		{
			vertexPos = gl_in[i].gl_Position;
			gl_Position = WVP * vertexPos;
			position_FRAG_in = vec3(world * vertexPos);
			tex_FRAG_in = tex_GEO_in[i];
			normals_FRAG_in = vec3(world * vec4(normals_GEO_in[i], 0.0f));
			tangent_FRAG_in = vec3(world * vec4(tangent_GEO_in[i], 0.0f));
			EmitVertex();
		}
	}

	EndPrimitive();
}