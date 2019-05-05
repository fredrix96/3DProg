#version 440
in vec2 tex_FRAG_in;

layout (location = 0) out vec4 finalMat;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gMaterial;
uniform sampler2D gParticle;

uniform sampler2D DepthMapTexture;

uniform vec3 LightPosition;
uniform vec3 LightColor;
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;
uniform vec3 LightSpecular;

uniform vec3 camPos;

uniform mat4 world, lightSpaceMatrix;

vec4 lightSpace = lightSpaceMatrix * world * texture(gPosition, tex_FRAG_in);

float ShadowCalculation(vec4 lightSpace, vec3 normal)
{
	//den mer komplicerade bias används ifall vi skulle få mera konstig shadow acne eller skuggartefakter typ? vi har ju inga så ser ingen skillnad, eller så är koden fel ¯\_(ツ)_/¯
	vec3 normalShadow = normal;
    vec3 lightDirShadow = normalize(LightPosition);
	float bias = max(0.01*(1 - dot(normalShadow, lightDirShadow)), 0.005);

	vec2 poissonDisk[8] = vec2[] (
 		vec2( -0.94201624, -0.39906216 ),
		vec2( -0.282571, -0.023957 ),
  		vec2( 0.94558609, -0.76890725 ),
		vec2( 0.945738, -0.411756 ),
  		vec2( -0.094184101, -0.92938870 ),
		vec2( -0.552995, -0.216651 ),
  		vec2( 0.34495938, 0.29387760 ),
  		vec2( 0.783654, 0.318522 ) 
	);

	float shadow = 1.0f;
	for (int i = 0; i < 8; i++)
	{
  		if (texture( DepthMapTexture, lightSpace.xy + poissonDisk[i]/700.0 ).z  <  lightSpace.z - bias)
		{
    		shadow -= 0.1;
  		}
	}

	return shadow;	
}

void main () {
	vec3 finalPos = texture(gPosition, tex_FRAG_in).xyz;
	vec3 finalTex = texture(gMaterial, tex_FRAG_in).rgb;
	vec3 finalNorm = texture(gNormal, tex_FRAG_in).xyz;
	finalNorm = normalize(finalNorm);
    vec4 finalPart = texture(gParticle, tex_FRAG_in);

	float shadow = ShadowCalculation(lightSpace, finalNorm);

    // ambient
    vec3 ambient = LightAmbient * finalTex;

    // diffuse
    vec3 lightDir = normalize(LightPosition - finalPos);
    float diff = max(dot(lightDir, finalNorm), 0.0f);
    vec3 diffuse = LightDiffuse * diff * LightColor;

    // specular
    vec3 viewDir = normalize(camPos - finalPos);
    float spec = 0.0f;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(finalNorm, halfwayDir), 0.0f), 64.0f);
    vec3 specular = LightSpecular * spec * LightColor;  

    vec3 lighting = (ambient + shadow * (diffuse + specular)) * finalTex;
	finalMat = vec4(lighting, 1.0f) + finalPart;
}