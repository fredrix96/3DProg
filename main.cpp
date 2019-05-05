#include <windows.h>

#include <string>
#include <fstream>
#include <streambuf>
#include <vector>
#include <iostream>

#include <glew.h>
#include <gl/GL.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "camera.h"

#pragma warning(disable:4996)
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")

using namespace std;
using namespace glm;

HWND InitWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HGLRC CreateOpenGLContext(HWND wndHandle);

Camera cam;
POINT mousePos;

// OpenGL uses unsigned integers to keep track of
// created resources (shaders, vertices, textures, etc)
// For simplicity, we make them global here, but it is
// safe to put them in a class and pass around...
GLuint gVertexBuffer = 0;
GLuint gVertexBuffer2 = 0;
GLuint gUVbuffer = 0;
GLuint gUVbuffer2 = 0;
GLuint gNormalbuffer = 0; 
GLuint gNormalbuffer2 = 0;
GLuint gTangentbuffer = 0; 
GLuint gTangentbuffer2 = 0;
GLuint gVertexAttribute = 0;
GLuint gVertexAttribute2 = 0;
GLuint gVertexAttributeShadow = 0;

// BP
GLuint billVertexBuffer;
GLuint pPosBuffer;
GLuint pColorBuffer;
GLuint gParticleTexture;
GLuint particleShader = 0;
GLuint particleVs = 0;
GLuint particleFs = 0;
GLuint gVertexAttributeParticle = 0;

// BBO
GLuint gVertexAttributeBBO = 0;
GLuint BBOVs = 0;
GLuint BBOFs = 0;
GLuint BBOProgram = 0;
GLuint vbo_vertices = 0;
GLuint ibo_elements = 0;

// geometry pass
GLuint gShaderProgram = 0;
GLuint vs = 0;
GLuint tcs = 0;
GLuint tes = 0;
GLuint gs = 0;
GLuint fs = 0;

// Gbuffer
GLuint GBuffer = 0;
GLuint gPosition, gNormal, gMaterial, gParticle = 0;
GLuint rboDepth = 0;

// lightPass
GLuint lightVs = 0;
GLuint lightFs = 0;
GLuint lightShaderProgram = 0;

// depth map pass
GLuint shadowShader = 0;
GLuint shadowVs = 0;
GLuint shadowFs = 0;
GLuint depthMapFBO = 0;
GLuint depthMapTex = 0;

// textures
GLuint gTexture = 0;
GLuint gNormalMap = 0;
GLuint gTexture2 = 0;
GLuint gNormalMap2 = 0;

// occlusion query
GLuint query = 0;

// gauss
GLuint sceneFBO;
GLuint sceneTexture;
GLuint depthrenderbuffer;
GLuint pingpongFBO[2];
GLuint pingpongBuffer[2];
GLuint blurShaderProgram = 0;
GLuint blurVertexShader = 0;
GLuint blurFragShader = 0;
GLuint quadVAO = 0;
GLuint quadVBO;
GLuint quadShaderProgram = 0;
GLuint quadVertexShader = 0;
GLuint quadFragShader = 0;

// locations
GLint matrixLocProjBBO, matrixLocViewBBO, matrixLocWorldBBO = 0;
GLint camPosLocLight = 0;
GLint matrixLocWorldLight = 0;
GLint depthMapTexLoc = 0;
GLint directionalLightLoc = 0;
GLint lightColorLoc, lightAmbLoc, lightDiffLoc, lightSpecLoc = 0;
GLint lightSpaceMatrixLoc = 0;
GLint textureLoc = 0;
GLint normalMapLoc = 0;
GLint camPosLoc = 0;
GLint matrixLocProj, matrixLocView, matrixLocWorld = 0;
GLint lightSpaceMatrixLocShadow = 0;
GLint matrixLocWorldShadow = 0;
GLuint camRightLoc, camUpLoc = 0;
GLuint particleViewLoc, particleProjLoc, particleTextureLoc = 0;
GLfloat camPos[3], lightPos[3], lightCol[3], lightAmb[3], lightDiff[3], lightSpec[3] = { 0.0, 0.0, 0.0 };

#define VIEWPORTHEIGHT 1024
#define VIEWPORTWIDTH 1024
#define WINDOWHEIGHT 1024
#define WINDOWWIDTH 1024
#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048
#define glPI 3.14159
#define FILL 100
#define RGBA 4
#define MAXPARTICLES 1000

// function to initiate a vertex- fragment shader pipeline
void InitiateShader(GLuint &vsName, string vsPath, GLuint &fsName, string fsPath, GLuint &programName);

// shader pipeline with vertex, tesselation, geometry and fragmentshader
void CreateShaders();

// load object and material
bool LoadObj(const char * objPath, vector<vec3> & out_vertices, vector<vec2> & out_UVs, vector<vec3> & out_normals, char & out_mtl);
bool LoadMtl(const char * mtlPath, char & out_diffuseTexMap);

// normal mapping
vector<vec3> CreateTangent(vector<vec3> vertices, vector<vec2> UVs, vector<vec3> normal);

// prepare object and texture data
vector<vec3> CreateTriangleData(int & nrOfVertices_out, int & nrOfVertices2_out, char & mtlFile, char & mtlFile2);
void TextureCreator(int width, int height, unsigned char * textureData, GLuint * texture);

// render object
void Render(int nrOfVertices, GLint textureLoc, GLint normalMapLoc,
	GLint depthMapTexLoc, GLuint vertexAttribute, GLuint texture, GLuint normalMap);

void LightPass();

// shadowmapping - create fbo and texture for the depthmap
void createDepthMap();

// shadowmapping
void renderToDepthMap(int nrOfVertices);

// gauss - framebuffer to render scene to
void createSceneFBO();

// gauss - pingpong buffers
void pingPongBuffers();

// gauss - quad to render the blurred texture to
void renderQuad();

// gauss - sends the scene-texture to the pingpongbuffers and blurs the texture horizontally/vertically every other pass
void gaussianBlur(int passes);

// BP
struct Particle {
    vec3 pos, speed;
    char r, g, b, a; //Color
    float size, angle, weight;
    float life; //Remaining life of the particle. if < 0: dead and unused
    float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

    bool operator<(const Particle& that) const {
        // Sort in reverse order : far particles drawn first.
        return this->cameradistance > that.cameradistance;
    }
};
Particle ParticlesContainer[MAXPARTICLES];
int LastUsedParticle = 0;
// Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
int FindUnusedParticle();

// BP
void CreateParticleData();

//Gbuffer
void configureGBuffer();

//Bounding box BBO
mat4 configureBBO(vector<vec3> & in_vertices);
void draw_bbox(mat4 world, GLuint worldLoc, mat4 view, GLuint viewLoc, mat4 proj, GLuint projLoc);

void SetViewport();
void MouseMovement();
void KeyMovement();
void deleteGlobalData();
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{	
	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hInstance); // 1. Skapa fönster
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	int nrOfVertices, nrOfVertices2;
	vector<vec3> vertices;
	char *mtlFile = new char[FILL];
	char *mtlFile2 = new char[FILL];
	char *texMap = new char[FILL];
	char *texMap2 = new char[FILL];
	unsigned char *textureData, *normalMapData, *textureData2, *normalMapData2;
	int tWidth, tHeight, tNrChannels, tWidth2, tHeight2, tNrChannels2;
	int nWidth, nHeight, nNrChannels, nWidth2, nHeight2, nNrChannels2;
	bool check;

	// BP
	unsigned char *particleTextureData;
	int pWidth, pHeight, pNrChannels;
	float lastRealTime = 0.0f;

	//FPS
	double lastTime = glfwGetTime();
	int frames = 0;
	char FPSstring[FILL];

    float time = 0;

    struct DirectionalLight {
        vec3 lightPos = vec3(-2.0f, 5.0f, -5.0f);
        vec3 lightCol = vec3(1.0f, 1.0f, 1.0f);
        vec3 lightAmb = vec3(0.5f, 0.5f, 0.5f);
        vec3 lightDiff = vec3(1.0f, 1.0f, 1.0f);
        vec3 lightSpec = vec3(1.0f, 1.0f, 1.0f);
    } DirectionalLight;

if (wndHandle)
{
		HDC hDC = GetDC(wndHandle);
		HGLRC hRC = CreateOpenGLContext(wndHandle); //2. Skapa och koppla OpenGL context

		glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_LESS);
		//enable cull face för att kunna ändra front/back före och efter shadowrender för att fixa "peter panning", vi har dock inte det problemet typ
		//glEnable(GL_CULL_FACE);

		glewInit(); //3. Initiera The OpenGL Extension Wrangler Library (GLEW)
		glfwInit(); //makes the time start in GLFW

		// generates query
		glGenQueries(1, &query);

		// which OpenGL did we get?
		GLint glMajor, glMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
		glGetIntegerv(GL_MINOR_VERSION, &glMinor);
		{
			char buff[64] = {};
			printf(buff, "OpenGL context version %d.%d created\n", glMajor, glMinor);
			OutputDebugStringA(buff);
		}

		SetViewport(); // Sätt viewport

		CreateShaders(); // Skapa vertex, tcs, tes, geometry- och fragment-shaders till geometry pass

		// lightpass shaders
		InitiateShader(lightVs, "LightVertex.glsl", lightFs, "LightFrag.glsl", lightShaderProgram);

		// BBO
		InitiateShader(BBOVs, "BBOVs.glsl", BBOFs, "BBOFs.glsl", BBOProgram);

		// shadowmapping
        InitiateShader(shadowVs, "ShadowVs.glsl", shadowFs, "ShadowFs.glsl", shadowShader);

        // Definiera triangelvertiser. Skapa vertex buffer object (VBO). Skapa vertex array object (VAO)				
		vertices = CreateTriangleData(nrOfVertices, nrOfVertices2, *mtlFile, *mtlFile2);
		
		configureGBuffer();

        // gauss
        InitiateShader(blurVertexShader, "gaussVertexShader.glsl", blurFragShader, "gaussFragShader.glsl", blurShaderProgram);
        InitiateShader(quadVertexShader, "quadVertexShader.glsl", quadFragShader, "quadFragShader.glsl", quadShaderProgram);
        pingPongBuffers();
        createSceneFBO();

		// BP
		CreateParticleData();
        InitiateShader(particleVs, "ParticleVs.glsl", particleFs, "ParticleFs.glsl", particleShader);

        // load material and check for error
		check = LoadMtl(mtlFile, *texMap);
		check = LoadMtl(mtlFile2, *texMap2);

		if (check == false) {
			OutputDebugStringA("Error, can not load material!\n");
		}
		
        // load texture data
		textureData = stbi_load(texMap, &tWidth, &tHeight, &tNrChannels, RGBA);
		textureData2 = stbi_load(texMap2, &tWidth2, &tHeight2, &tNrChannels2, RGBA);

		if (textureData == NULL || textureData2 == NULL) {
			OutputDebugStringA("Error, can not load texture!\n");
		}

		normalMapData = stbi_load("ground_normal.png", &nWidth, &nHeight, &nNrChannels, RGBA);
		normalMapData2 = stbi_load("normal_map.png", &nWidth2, &nHeight2, &nNrChannels2, RGBA);

		if (normalMapData == NULL || normalMapData2 == NULL) {
			OutputDebugStringA("Error, can not load normal map!\n");
		}

        //creating the textures
        TextureCreator(tWidth, tHeight, textureData, &gTexture);
        TextureCreator(tWidth2, tHeight2, textureData2, &gTexture2);

        //creating the normalmaps
        TextureCreator(nWidth, nHeight, normalMapData, &gNormalMap);
        TextureCreator(nWidth2, nHeight2, normalMapData2, &gNormalMap2);

        //skapa texturen som ska användas som depth buffer till shadow map
        createDepthMap();

		// BP
		particleTextureData = stbi_load("snowflake2.png", &pWidth, &pHeight, &pNrChannels, RGBA);
		if (particleTextureData == NULL) {
			OutputDebugStringA("Error, can not load particle texture!\n");
		}

		// BP
		static GLfloat * particlePositionSizeData = new GLfloat[MAXPARTICLES * 4];
		static GLfloat * particleColorSizeData = new GLfloat[MAXPARTICLES * 4];
		for (int i = 0; i < MAXPARTICLES; i++) {
			ParticlesContainer[i].life = -1.0f;
			ParticlesContainer[i].cameradistance = -1.0f;
		}

        // BP
        TextureCreator(pWidth, pHeight, particleTextureData, &gParticleTexture);

		ShowWindow(wndHandle, nCmdShow);

        //_______________________________________________________________________________________________________________________
        // get locations

        // gauss
        glUseProgram(blurShaderProgram);
        glUniform1i(glGetUniformLocation(blurShaderProgram, "image"), 0);

        glUseProgram(quadShaderProgram);
        glUniform1i(glGetUniformLocation(quadShaderProgram, "blurredTexture"), 0);

		// BBO
        glUseProgram(BBOProgram);
		matrixLocProjBBO = glGetUniformLocation(BBOProgram, "proj");
		matrixLocViewBBO = glGetUniformLocation(BBOProgram, "view");
		matrixLocWorldBBO = glGetUniformLocation(BBOProgram, "world");
		mat4 BBOworld = configureBBO(vertices);

		// lightpass
		glUseProgram(lightShaderProgram);
		camPosLocLight = glGetUniformLocation(lightShaderProgram, "camPos");
		matrixLocWorldLight = glGetUniformLocation(lightShaderProgram, "world");
		depthMapTexLoc = glGetUniformLocation(lightShaderProgram, "DepthMapTexture");
		directionalLightLoc = glGetUniformLocation(lightShaderProgram, "LightPosition");
		lightColorLoc = glGetUniformLocation(lightShaderProgram, "LightColor");
		lightAmbLoc = glGetUniformLocation(lightShaderProgram, "LightAmbient");
		lightDiffLoc = glGetUniformLocation(lightShaderProgram, "LightDiffuse");
		lightSpecLoc = glGetUniformLocation(lightShaderProgram, "LightSpecular");
		lightSpaceMatrixLoc = glGetUniformLocation(lightShaderProgram, "lightSpaceMatrix");

        // geometry pass
        glUseProgram(gShaderProgram);
		textureLoc = glGetUniformLocation(gShaderProgram, "Texture");
		normalMapLoc = glGetUniformLocation(gShaderProgram, "NormalMap");
		camPosLoc = glGetUniformLocation(gShaderProgram, "camPos");
		matrixLocProj = glGetUniformLocation(gShaderProgram, "proj");
		matrixLocView = glGetUniformLocation(gShaderProgram, "view");
		matrixLocWorld = glGetUniformLocation(gShaderProgram, "world");

        glUseProgram(shadowShader);
		lightSpaceMatrixLocShadow = glGetUniformLocation(shadowShader, "lightSpaceMatrix");
		matrixLocWorldShadow = glGetUniformLocation(shadowShader, "world");

		// BP
        glUseProgram(particleShader);
		camRightLoc = glGetUniformLocation(particleShader, "camRight");
		camUpLoc = glGetUniformLocation(particleShader, "camUp");
		particleViewLoc = glGetUniformLocation(particleShader, "view");
		particleProjLoc = glGetUniformLocation(particleShader, "proj");
		particleTextureLoc = glGetUniformLocation(particleShader, "particleTexture");

        // initiate stuff
		mat4 projMatrix = perspective(radians(80.0f), (float)(VIEWPORTWIDTH / VIEWPORTHEIGHT), 0.01f, 20.0f);
		mat4 viewMatrix;

		mat4 worldTranslation, worldScale, worldMatrix;

		mat4 lightProjection = ortho<float>(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 20.0f);
		mat4 lightView = lookAt(DirectionalLight.lightPos, vec3(0, 0, 0), vec3(0, 1, 0));
		mat4 lightSpaceMatrix = lightProjection * lightView;

		mat4 biasMatrix(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f
		);

		mat4 depthBiasMVP;
		
        lightPos[0] = DirectionalLight.lightPos.x;
        lightPos[1] = DirectionalLight.lightPos.y;
        lightPos[2] = DirectionalLight.lightPos.z;
        lightCol[0] = DirectionalLight.lightCol.x;
        lightCol[1] = DirectionalLight.lightCol.y;
        lightCol[2] = DirectionalLight.lightCol.z;
        lightAmb[0] = DirectionalLight.lightAmb.x;
        lightAmb[1] = DirectionalLight.lightAmb.y;
        lightAmb[2] = DirectionalLight.lightAmb.z;
        lightDiff[0] = DirectionalLight.lightDiff.x;
        lightDiff[1] = DirectionalLight.lightDiff.y;
        lightDiff[2] = DirectionalLight.lightDiff.z;
        lightSpec[0] = DirectionalLight.lightSpec.x;
        lightSpec[1] = DirectionalLight.lightSpec.y;
        lightSpec[2] = DirectionalLight.lightSpec.z;

        // up vector to particles
        vec3 up = vec3(0.0f, 1.0f, 0.0f);
        float delta = 0;
        vec3 CameraPosition = vec3(0, 0, 0);

        // how many blur passes
        int passes = 0;

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
                if (GetKeyState('O') & 0x8000)
                {
                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

                    //visa polygons! duger nog att aktivera för att bevisa att tesselation lod funkar
                    glPolygonMode(GL_FRONT, GL_LINE);

                    glUseProgram(gShaderProgram);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glClearColor(1, 1, 1, 1);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    //getting the camera position
                    camPos[0] = cam.getCamPos().x;
                    camPos[1] = cam.getCamPos().y;
                    camPos[2] = cam.getCamPos().z;

                    //moving the camera = viewing transformation
                    viewMatrix = cam.getViewMatrix();

                    float time = float(glfwGetTime());

                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Golvet

                    //moving the subject = model/world transformation
                    worldTranslation = translate(mat4(), vec3(0.0f, 0.0f, 0.0f));
                    worldScale = scale(worldTranslation, vec3(1.0f, 1.0f, 1.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);
                    glUniformMatrix4fv(matrixLocView, 1, GL_FALSE, &viewMatrix[0][0]);

                    //camera position
                    glUniform3fv(camPosLoc, 1, camPos);

                    //WorldViewProjection
                    glUniformMatrix4fv(matrixLocProj, 1, GL_FALSE, &projMatrix[0][0]);

                    Render(nrOfVertices, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute, gTexture, gNormalMap2);

                    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Gubbe1 (lilla)

                    //moving the subject = model/world transformation
                    worldTranslation = translate(mat4(), vec3(0.0f, 0.02f, 2.0f));
                    worldScale = scale(worldTranslation, vec3(1.0f, 1.0f, 1.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);

                    Render(nrOfVertices2, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute2, gTexture2, gNormalMap2);

                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Gubbe2 (stora)

                    //moving the subject = model/world transformation
                    worldTranslation = translate(mat4(), vec3(0.0f, 0.02f, -2.0f));
                    worldScale = scale(worldTranslation, vec3(2.0f, 2.0f, 2.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);

                    Render(nrOfVertices2, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute2, gTexture2, gNormalMap2);

                    // ljuset i rörelse
                    DirectionalLight.lightPos.x = sinf((float)glfwGetTime() * 0.5f * (float)glPI) * 5.0f;
                    DirectionalLight.lightPos.z = cosf((float)glfwGetTime() * 0.5f * (float)glPI) * 5.0f;

                    MouseMovement();
                    KeyMovement();

                    SwapBuffers(hDC); //10. Växla front- och back-buffer

                    glPolygonMode(GL_FRONT, GL_FILL);
                }
                else
                {
                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

                    glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);

                    glUseProgram(gShaderProgram);

                    glClearColor(0, 0, 0, 1);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    //getting the camera position
                    camPos[0] = cam.getCamPos().x;
                    camPos[1] = cam.getCamPos().y;
                    camPos[2] = cam.getCamPos().z;

                    //moving the camera = viewing transformation
                    viewMatrix = cam.getViewMatrix();

                    time = float(glfwGetTime());

                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Golvet
                    glUseProgram(gShaderProgram);

                    //moving the subject = model/world transformation
                    worldTranslation = translate(mat4(), vec3(0.0f, 0.0f, 0.0f));
                    worldScale = scale(worldTranslation, vec3(1.0f, 1.0f, 1.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);
                    glUniformMatrix4fv(matrixLocWorldShadow, 1, GL_FALSE, &worldMatrix[0][0]);

                    lightView = lookAt(DirectionalLight.lightPos, vec3(0, 0, 0), vec3(0.0f, 1.0f, 0.0f));
                    lightSpaceMatrix = lightProjection * lightView;

                    glUniformMatrix4fv(matrixLocView, 1, GL_FALSE, &viewMatrix[0][0]);

                    //camera position
                    glUniform3fv(camPosLoc, 1, camPos);

                    //WorldViewProjection
                    glUniformMatrix4fv(matrixLocProj, 1, GL_FALSE, &projMatrix[0][0]);

                    glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);
                    Render(nrOfVertices, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute, gTexture, gNormalMap); //9. Rendera

                    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Gubbe1 stora gubben
                    glUseProgram(shadowShader);
                    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

                    //Cleara depthbuffer
                    glClear(GL_DEPTH_BUFFER_BIT);

                    //moving the subject = model/world transformation
                    worldTranslation = translate(mat4(), vec3(0.0f, 0.02f, -2.0f));
                    worldScale = scale(worldTranslation, vec3(2.0f, 2.0f, 2.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);

                    //skicka matrisen från ljusets perspektiv
                    glUniformMatrix4fv(lightSpaceMatrixLocShadow, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

                    //skicka worldmatris till shadowshader
                    glUniformMatrix4fv(matrixLocWorldShadow, 1, GL_FALSE, &worldMatrix[0][0]);

                    renderToDepthMap(nrOfVertices2);

                    glUseProgram(gShaderProgram);

                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);

                    glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);
                    Render(nrOfVertices2, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute2, gTexture2, gNormalMap2);

                    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Gubbe2 lilla gubben

                    glUseProgram(shadowShader);

                    worldTranslation = translate(mat4(), vec3(0.0f, 0.02f, 2.0f));
                    worldScale = scale(worldTranslation, vec3(1.0f, 1.0f, 1.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);

                    //skicka matrisen från ljusets perspektiv
                    glUniformMatrix4fv(lightSpaceMatrixLocShadow, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
                    //skicka worldmatris till shadowshader
                    glUniformMatrix4fv(matrixLocWorldShadow, 1, GL_FALSE, &worldMatrix[0][0]);

                    renderToDepthMap(nrOfVertices2);

                    glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);

					if (GetKeyState('P') & 0x8000) {
						glUseProgram(gShaderProgram);

						glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);

						Render(nrOfVertices2, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute2, gTexture2, gNormalMap2);
					}
					else {
						glUseProgram(BBOProgram);

						glColorMask(false, false, false, false);
						glDepthMask(false);
						glBeginQuery(GL_SAMPLES_PASSED, query);
                            worldMatrix = worldMatrix * BBOworld;
							draw_bbox(worldMatrix, matrixLocWorldBBO, viewMatrix, matrixLocViewBBO, projMatrix, matrixLocProjBBO);
						glEndQuery(GL_SAMPLES_PASSED);
						glColorMask(true, true, true, true);
						glDepthMask(true);

						glUseProgram(gShaderProgram);
						glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);

						GLint nr = 0;
						char s[100];
						glGetQueryObjectiv(query, GL_QUERY_RESULT, &nr);
						sprintf(s, "There are %d numbers of visible pixels\n", nr);
						if (GetKeyState('V') & 0x8000) {
							OutputDebugStringA(s);
						}

						if (nr > 0) {
							worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
							glUniformMatrix4fv(matrixLocWorld, 1, GL_FALSE, &worldMatrix[0][0]);
							Render(nrOfVertices2, textureLoc, normalMapLoc, depthMapTexLoc, gVertexAttribute2, gTexture2, gNormalMap2);
						}
					}

                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    
                    // BP
                    delta = (time - lastRealTime) / 5;
                    CameraPosition = cam.getCamPos();
                    // Generate 10 new particule each millisecond,
                    // but limit this to 16 ms (60 fps), or if you have 1 long frame (1sec),
                    // newparticles will be huge and the next frame even longer.
                    int newparticles = (int)(delta * 10000.0);
                    if (newparticles > (int)(0.016f * 10000.0))
                        newparticles = (int)(0.016f * 10000.0);

                    for (int i = 0; i < newparticles; i++) {
                        int particleIndex = FindUnusedParticle();
                        ParticlesContainer[particleIndex].life = 5.0f; // This particle will live 5 seconds.
                        ParticlesContainer[particleIndex].pos = vec3((rand() % 16) - 8, rand() % 16, (rand() % 16) - 8); //Startpos

                        float spread = 1.5f;
                        vec3 maindir = vec3(0.0f, -1.0f, 0.0f);
                        // generate a random direction; 
                        vec3 randomdir = vec3(
                            (rand() % 2000 - 1000.0f) / 1000.0f,
                            (rand() % 2000 - 1000.0f) / 1000.0f,
                            (rand() % 2000 - 1000.0f) / 1000.0f
                        );

                        ParticlesContainer[particleIndex].speed = maindir + randomdir * spread;

                        // generate a random color
                        //ParticlesContainer[particleIndex].r = rand() % 256;
                        //ParticlesContainer[particleIndex].g = rand() % 256;
                        //ParticlesContainer[particleIndex].b = rand() % 256;
                        //ParticlesContainer[particleIndex].a = (rand() % 256) / 3;
                        // white
                        ParticlesContainer[particleIndex].r = 1;
                        ParticlesContainer[particleIndex].g = 1;
                        ParticlesContainer[particleIndex].b = 1;
                        ParticlesContainer[particleIndex].a = 1;

                        ParticlesContainer[particleIndex].size = (rand() % 1000) / 20000.0f + 0.1f;
                    }

                    // Simulate all particles
                    int ParticlesCount = 0;
                    for (int i = 0; i < MAXPARTICLES; i++) {

                        Particle& p = ParticlesContainer[i]; // shortcut

                        if (p.life > 0.0f) {

                            // Decrease life
                            p.life -= delta;
                            if (p.life > 0.0f) {

                                // Simulate simple physics : gravity only, no collisions
                                p.speed += vec3(0.0f, -9.81f, 0.0f) * (float)delta * 0.05f;
                                p.pos += p.speed * (float)delta;
                                p.cameradistance = length(p.pos - CameraPosition);

                                // Fill the GPU buffer
                                particlePositionSizeData[4 * ParticlesCount + 0] = p.pos.x;
                                particlePositionSizeData[4 * ParticlesCount + 1] = p.pos.y;
                                particlePositionSizeData[4 * ParticlesCount + 2] = p.pos.z;

                                particlePositionSizeData[4 * ParticlesCount + 3] = p.size / p.cameradistance;

                                particleColorSizeData[4 * ParticlesCount + 0] = p.r;
                                particleColorSizeData[4 * ParticlesCount + 1] = p.g;
                                particleColorSizeData[4 * ParticlesCount + 2] = p.b;
                                particleColorSizeData[4 * ParticlesCount + 3] = p.a;

                            }
                            else {
                                // Particles that just died will be put at the end of the buffer in SortParticles();
                                p.cameradistance = -1.0f;
                            }

                            ParticlesCount++;
                        }
                    }

                    // BP
                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);

                    //Partiklar
                    //Updating the buffers

                    glBindBuffer(GL_ARRAY_BUFFER, pPosBuffer);
                    glBufferData(GL_ARRAY_BUFFER, MAXPARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming performance
                    glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, particlePositionSizeData);

                    glBindBuffer(GL_ARRAY_BUFFER, pColorBuffer);
                    glBufferData(GL_ARRAY_BUFFER, MAXPARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, particleColorSizeData);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    glUseProgram(particleShader);

                    glBindVertexArray(gVertexAttributeParticle);

                    glUniformMatrix4fv(particleViewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
                    glUniformMatrix4fv(particleProjLoc, 1, GL_FALSE, &projMatrix[0][0]);
                    glUniform3f(camRightLoc, viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
                    glUniform3f(camUpLoc, viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, gParticleTexture);
                    glUniform1i(particleTextureLoc, 0);

                    // These functions are specific to glDrawArrays*Instanced*.
                    // The first parameter is the attribute buffer we're talking about.
                    // The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
                    // http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
                    glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
                    glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
                    glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

                    // Draw the particules! This draws many times a small triangle_strip (which looks like a quad).
                    // This is equivalent to : for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), but faster
                    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

                    glBindVertexArray(0);
                    glDisable(GL_BLEND);

                    glUseProgram(lightShaderProgram);
                    LightPass();

                    depthBiasMVP = biasMatrix * lightSpaceMatrix;

                    worldTranslation = translate(mat4(), vec3(0.0f, 0.0f, 0.0f));
                    worldScale = scale(worldTranslation, vec3(1.0f, 1.0f, 1.0f));
                    worldMatrix = rotate(worldScale, 0.0f, vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(matrixLocWorldLight, 1, GL_FALSE, &worldMatrix[0][0]);
                    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, &depthBiasMVP[0][0]);

                    renderQuad();                    

                    gaussianBlur(passes);

                    if (GetKeyState('E') & 0x8000) {
                        passes += 1;
                    }
                    if (GetKeyState('Q') & 0x8000) {
						if (passes != 0)
						{
							passes -= 1;
						}
                    }

                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                    //test med ljuset i rörelse
                    DirectionalLight.lightPos.x = sinf((float)glfwGetTime() * 0.5f * (float)glPI) * 5.0f;
                    DirectionalLight.lightPos.z = cosf((float)glfwGetTime() * 0.5f * (float)glPI) * 5.0f;

                    MouseMovement();
                    KeyMovement();

                    //FPS
                    //measure speed
                    double currentTime = glfwGetTime();
                    frames++;

                    //if last sprinf() was more than 1 sec ago, sprintf and reset timer
                    if ((currentTime - lastTime) >= 1.0f) {
                        sprintf(FPSstring, "%d FPS\n", frames);
                        if (GetKeyState('F') & 0x8000) {
                            OutputDebugStringA(FPSstring);
                        }
                        frames = 0;
                        lastTime += 1.0f;
                    }

                    lastRealTime = time;
                    SwapBuffers(hDC); //10. Växla front- och back-buffer
                }
			}
		}

		deleteGlobalData();

		delete[] particlePositionSizeData;
		delete[] particleColorSizeData;
		delete mtlFile;
		delete texMap;
		delete mtlFile2;
		delete texMap2;

		stbi_image_free(textureData);
		stbi_image_free(normalMapData);
		stbi_image_free(textureData2);
		stbi_image_free(normalMapData2);
		stbi_image_free(particleTextureData);

		glfwTerminate();

		// release OpenGL context
		wglMakeCurrent(NULL, NULL);
		// release device context handle
		ReleaseDC(wndHandle, hDC);
		// delete context
		wglDeleteContext(hRC);
		// kill window
		DestroyWindow(wndHandle);
	}

	return (int) msg.wParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Win32 specific code. Create a window with certain characteristics
HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.lpszClassName = "BTH_GL_DEMO";
	if( !RegisterClassEx(&wcex) )
		return false;

	// window size
	RECT rc = { 0, 0, WINDOWWIDTH, WINDOWHEIGHT};
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

	// win32 call to create a window
	HWND handle = CreateWindow(
		"BTH_GL_DEMO",
		"BTH OpenGL Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);
	return handle;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;		
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HGLRC CreateOpenGLContext(HWND wndHandle)
{
	//get handle to a device context (DC) for the client area
	//of a specified window or for the entire screen
	HDC hDC = GetDC(wndHandle);

	//details: http://msdn.microsoft.com/en-us/library/windows/desktop/dd318286(v=vs.85).aspx
	static  PIXELFORMATDESCRIPTOR pixelFormatDesc =
	{
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd  
		1,                                // version number  
		PFD_DRAW_TO_WINDOW |              // support window  
		PFD_SUPPORT_OPENGL |              // support OpenGL  
		PFD_DOUBLEBUFFER,                 // double buffered
		//PFD_DEPTH_DONTCARE,             // disable depth buffer <-- added by Stefan
		PFD_TYPE_RGBA,                    // RGBA type  
		32,                               // 32-bit color depth (4*8)  
		0, 0, 0, 0, 0, 0,                 // color bits ignored  
		0,                                // no alpha buffer  
		0,                                // shift bit ignored  
		0,                                // no accumulation buffer  
		0, 0, 0, 0,                       // accum bits ignored  
		24,    // 24                       // 0-bits for depth buffer <-- modified by Stefan      
		8,    // 8                        // no stencil buffer  
		0,                                // no auxiliary buffer  
		PFD_MAIN_PLANE,                   // main layer  
		0,                                // reserved  
		0, 0, 0                           // layer masks ignored  
	};

	//attempt to match an appropriate pixel format supported by a
	//device context to a given pixel format specification.
	int pixelFormat = ChoosePixelFormat(hDC, &pixelFormatDesc);

	//set the pixel format of the specified device context
	//to the format specified by the iPixelFormat index.
	SetPixelFormat(hDC, pixelFormat, &pixelFormatDesc);

	//create a new OpenGL rendering context, which is suitable for drawing
	//on the device referenced by hdc. The rendering context has the same
	//pixel format as the device context.
	HGLRC hRC = wglCreateContext(hDC);
	
	//makes a specified OpenGL rendering context the calling thread's current
	//rendering context. All subsequent OpenGL calls made by the thread are
	//drawn on the device identified by hdc. 
	wglMakeCurrent(hDC, hRC);

	return hRC;
}

// function to initiate a vertex- fragment shader pipeline
void InitiateShader(GLuint &vsName, string vsPath, GLuint &fsName, string fsPath, GLuint &programName)
{
    // local buffer to store error strings when compiling.
    char buff[1024];
    memset(buff, 0, 1024);
    GLint compileResult = 0;

    //create vertex shader "name" and store it in "vs"
    vsName = glCreateShader(GL_VERTEX_SHADER);

    // open .glsl file and put it in a string
    ifstream shaderFile(vsPath);
    std::string shaderText((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();

    // glShaderSource requires a double pointer.
    // get the pointer to the c style string stored in the string object.
    const char* shaderTextPtr = shaderText.c_str();

    // ask GL to use this string a shader code source
    glShaderSource(vsName, 1, &shaderTextPtr, nullptr);

    // try to compile this shader source.
    glCompileShader(vsName);

    // check for compilation error
    glGetShaderiv(vsName, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        // query information about the compilation (nothing if compilation went fine!)
        glGetShaderInfoLog(vsName, 1024, nullptr, buff);
        // print to Visual Studio debug console output
        OutputDebugStringA(buff);
    }

    // repeat process for Fragment Shader (or Pixel Shader)
    fsName = glCreateShader(GL_FRAGMENT_SHADER);
    shaderFile.open(fsPath);
    shaderText.assign((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();
    shaderTextPtr = shaderText.c_str();
    glShaderSource(fsName, 1, &shaderTextPtr, nullptr);
    glCompileShader(fsName);
    // query information about the compilation (nothing if compilation went fine!)
    compileResult = GL_FALSE;
    glGetShaderiv(fsName, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        // query information about the compilation (nothing if compilation went fine!)
        memset(buff, 0, 1024);
        glGetShaderInfoLog(fsName, 1024, nullptr, buff);
        // print to Visual Studio debug console output
        OutputDebugStringA(buff);
    }

    //link shader program (connect vs and ps)
    programName = glCreateProgram();
    glAttachShader(programName, fsName);
    glAttachShader(programName, vsName);
    glLinkProgram(programName);

    // check once more, if the Vertex Shader and the Fragment Shader can be used together
    compileResult = GL_FALSE;
    glGetProgramiv(programName, GL_LINK_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        // query information about the compilation (nothing if compilation went fine!)
        memset(buff, 0, 1024);
        glGetProgramInfoLog(programName, 1024, nullptr, buff);
        // print to Visual Studio debug console output
        OutputDebugStringA(buff);
    }
    // in any case (compile sucess or not), we only want to keep the 
    // Program around, not the shaders.
    glDetachShader(programName, vsName);
    glDetachShader(programName, fsName);
    glDeleteShader(vsName);
    glDeleteShader(fsName);
}

// shader pipeline with vertex, tesselation, geometry and fragmentshader
void CreateShaders()
{
    // local buffer to store error strings when compiling.
    char buff[1024];
    memset(buff, 0, 1024);
    GLint compileResult = 0;

    //create vertex shader "name" and store it in "vs"
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);

    // open .glsl file and put it in a string
    ifstream shaderFile("VertexShader.glsl");
    string shaderText((istreambuf_iterator<char>(shaderFile)), istreambuf_iterator<char>());
    shaderFile.close();

    // glShaderSource requires a double pointer.
    // get the pointer to the c style string stored in the string object.
    const char* shaderTextPtr = shaderText.c_str();

    // ask GL to use this string a shader code source
    glShaderSource(vs, 1, &shaderTextPtr, nullptr);

    // try to compile this shader source.
    glCompileShader(vs);

    // check for compilation error
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        // query information about the compilation (nothing if compilation went fine!)
        glGetShaderInfoLog(vs, 1024, nullptr, buff);
        // print to Visual Studio debug console output
        OutputDebugStringA(buff);
    }

    // repeat process for the other shaders
    GLuint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
    shaderFile.open("TessellationControlShader.glsl");
    shaderText.assign((istreambuf_iterator<char>(shaderFile)), istreambuf_iterator<char>());
    shaderFile.close();
    shaderTextPtr = shaderText.c_str();
    glShaderSource(tcs, 1, &shaderTextPtr, nullptr);
    glCompileShader(tcs);
    compileResult = GL_FALSE;
    glGetShaderiv(tcs, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        memset(buff, 0, 1024);
        glGetShaderInfoLog(tcs, 1024, nullptr, buff);
        OutputDebugStringA(buff);
    }

    GLuint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
    shaderFile.open("TessellationEvaluationShader.glsl");
    shaderText.assign((istreambuf_iterator<char>(shaderFile)), istreambuf_iterator<char>());
    shaderFile.close();
    shaderTextPtr = shaderText.c_str();
    glShaderSource(tes, 1, &shaderTextPtr, nullptr);
    glCompileShader(tes);
    compileResult = GL_FALSE;
    glGetShaderiv(tes, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        memset(buff, 0, 1024);
        glGetShaderInfoLog(tes, 1024, nullptr, buff);
        OutputDebugStringA(buff);
    }

    GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
    shaderFile.open("GeometryShader.glsl");
    shaderText.assign((istreambuf_iterator<char>(shaderFile)), istreambuf_iterator<char>());
    shaderFile.close();
    shaderTextPtr = shaderText.c_str();
    glShaderSource(gs, 1, &shaderTextPtr, nullptr);
    glCompileShader(gs);
    compileResult = GL_FALSE;
    glGetShaderiv(gs, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        memset(buff, 0, 1024);
        glGetShaderInfoLog(gs, 1024, nullptr, buff);
        OutputDebugStringA(buff);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    shaderFile.open("Fragment.glsl");
    shaderText.assign((istreambuf_iterator<char>(shaderFile)), istreambuf_iterator<char>());
    shaderFile.close();
    shaderTextPtr = shaderText.c_str();
    glShaderSource(fs, 1, &shaderTextPtr, nullptr);
    glCompileShader(fs);
    // query information about the compilation (nothing if compilation went fine!)
    compileResult = GL_FALSE;
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        // query information about the compilation (nothing if compilation went fine!)
        memset(buff, 0, 1024);
        glGetShaderInfoLog(fs, 1024, nullptr, buff);
        // print to Visual Studio debug console output
        OutputDebugStringA(buff);
    }

    //link shader program (connect vs and ps)
    gShaderProgram = glCreateProgram();
    glAttachShader(gShaderProgram, fs);
    glAttachShader(gShaderProgram, gs);
    glAttachShader(gShaderProgram, tes);
    glAttachShader(gShaderProgram, tcs);
    glAttachShader(gShaderProgram, vs);
    glLinkProgram(gShaderProgram);

    // check once more, if the Vertex Shader and the Fragment Shader can be used
    // together
    compileResult = GL_FALSE;
    glGetProgramiv(gShaderProgram, GL_LINK_STATUS, &compileResult);
    if (compileResult == GL_FALSE) {
        // query information about the compilation (nothing if compilation went fine!)
        memset(buff, 0, 1024);
        glGetProgramInfoLog(gShaderProgram, 1024, nullptr, buff);
        // print to Visual Studio debug console output
        OutputDebugStringA(buff);
    }
    // in any case (compile sucess or not), we only want to keep the 
    // Program around, not the shaders.
    glDetachShader(gShaderProgram, vs);
    glDetachShader(gShaderProgram, tcs);
    glDetachShader(gShaderProgram, tes);
    glDetachShader(gShaderProgram, gs);
    glDetachShader(gShaderProgram, fs);
    glDeleteShader(vs);
    glDeleteShader(tcs);
    glDeleteShader(tes);
    glDeleteShader(gs);
    glDeleteShader(fs);
}

bool LoadObj(const char * objPath, vector<vec3> & out_vertices, vector<vec2> & out_UVs, vector<vec3> & out_normals, char & out_mtl) {

    vector<unsigned int> vertexIndices, UVIndices, normalIndices;
    vector<vec3> tmp_vertices, tmp_normals;
    vector<vec2> tmp_UVs;

    FILE * file = fopen(objPath, "r");

    if (file == NULL) {
        printf("ERROR LOADING OBJECT! \n");
        return false;
    }

    while (1) { //runs until a break
                //reads the header of the file and assumes that
                //the first word wont be longer than 100
        char header[FILL];
        int res = fscanf(file, "%s", header);
        if (res == EOF) {
            break;
        }
        else {
            //if the first word of the line starts with "mtllib", then we will copy the string name
            //of the mtl file
            if (strcmp(header, "mtllib") == 0) {
                fscanf(file, "%s\n", &out_mtl);
            }
            //if the first word of the line is "v", then the rest
            //has to be 3 floats. So we will create a vec3 out of them
            //and add it to our vector
            else if (strcmp(header, "v") == 0) {
                vec3 vertex;
                fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
                tmp_vertices.push_back(vertex); //adds vertex to the end of our vector
            }
            //same with the UVs, but with just two floats and only if the line starts with "vt"
            else if (strcmp(header, "vt") == 0) {
                vec2 UV;
                fscanf(file, "%f %f\n", &UV.x, &UV.y);
                //have to flip the y-coordinates because openGl starts at the bottom
                UV.y = 1.0f - UV.y;
                tmp_UVs.push_back(UV);
            }
            //same with the normals
            else if (strcmp(header, "vn") == 0) {
                vec3 normal;
                fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
                tmp_normals.push_back(normal);
            }
            //finally doing the same with the lines starting with "f". Although the process is
            //similar to the others, this one needs you to look if the line matches with the amount
            //of data necessary to assign the unsigned ints.
            else if (strcmp(header, "f") == 0) {
                string v1, v2, v3;
                unsigned int vertexIndex[3], UVIndex[3], normalIndex[3];
                int amount = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                    &vertexIndex[0], &UVIndex[0], &normalIndex[0],
                    &vertexIndex[1], &UVIndex[1], &normalIndex[1],
                    &vertexIndex[2], &UVIndex[2], &normalIndex[2]);
                if (amount != 9) {
                    printf("ERROR! The object can not be read by the parser!\n");
                    return false;
                }
                for (unsigned int i = 0; i < 3; i++) {
                    vertexIndices.push_back(vertexIndex[i]);
                    UVIndices.push_back(UVIndex[i]);
                    normalIndices.push_back(normalIndex[i]);
                }
            }
        }
    }

    //we have now changed the shape of the incoming data to vectors, but now we need to
    //change the data from vector to vec3 which OpenGL is used to (Indexing). 

    //we need to go trough each vertex (v/vt/vn) of each triangle (f)
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {

        unsigned int vertexIndex = vertexIndices[i];

        //OBJ indexing starts at 1 while C++ starts at 0, therefore we need to substract 1
        vec3 vertex = tmp_vertices[vertexIndex - 1];
        out_vertices.push_back(vertex);

        //same with the UVs
        unsigned int UVIndex = UVIndices[i];
        vec2 UV = tmp_UVs[UVIndex - 1];
        out_UVs.push_back(UV);

        //same with the normals
        unsigned int normalIndex = normalIndices[i];
        vec3 normal = tmp_normals[normalIndex - 1];
        out_normals.push_back(normal);
    }
    return true;
}

bool LoadMtl(const char * mtlPath, char & out_diffuseTexMap) {

    FILE * file = fopen(mtlPath, "r");

    if (file == NULL) {
        printf("ERROR LOADING MATERIAL! \n");
        return false;
    }

    while (1) { //runs until a break
                //reads the header of the file and assumes that
                //the first word wont be longer than 100
        char header[FILL];
        int res = fscanf(file, "%s", header);
        if (res == EOF) {
            break;
        }
        else {
            //here we find the texture (material) to the object
            if (strcmp(header, "map_Kd") == 0) {
                fscanf(file, "%s\n", &out_diffuseTexMap);
            }
        }
    }
    return true;
}

vector<vec3> CreateTangent(vector<vec3> vertices, vector<vec2> UVs, vector<vec3> normal) {
    vector<vec3> tangent;

    for (unsigned int i = 0; i < vertices.size(); i += 3) {
        //calculating the triangles edges
        vec3 v0 = vertices[i];
        vec3 v1 = vertices[i + 1];
        vec3 v2 = vertices[i + 2];
        vec3 e0 = v1 - v0;
        vec3 e1 = v2 - v0;

        //calculating the distance between the UV coordinates
        vec2 UV0 = UVs[i];
        vec2 UV1 = UVs[i + 1];
        vec2 UV2 = UVs[i + 2];
        float distU0 = UV1.x - UV0.x;
        float distV0 = UV1.y - UV0.y;
        float distU1 = UV2.x - UV0.x;
        float distV1 = UV2.y - UV0.y;

        //calculating the difference between the UV distances
        float f = 1.0f / (distU0 * distV1 - distU1 * distV0);

        //calculating the tangent and bitangent
        vec3 tmpTangent;

        tmpTangent.x = f * (distV1 * e0.x - distV0 * e1.x);
        tmpTangent.y = f * (distV1 * e0.y - distV0 * e1.y);
        tmpTangent.z = f * (distV1 * e0.z - distV0 * e1.z);

        tangent.push_back(tmpTangent);
        tangent.push_back(tmpTangent);
        tangent.push_back(tmpTangent);
    }

    for (unsigned int i = 0; i < vertices.size(); i++) {
        tangent[i] = normalize(tangent[i]);
    }
    return tangent;
}

vector<vec3> CreateTriangleData(int & nrOfVertices_out, int & nrOfVertices2_out, char & mtlFile, char & mtlFile2)
{
    vector<vec3> vertices, vertices2, normals, normals2;
    vector<vec2> UVs, UVs2;
    char * object = "ground.obj";
    char * object2 = "Piedmon.obj";
    bool check;
    GLint maxPatchVertices = 0;

    //looking for the max amount of patchvertices that the GPU can handle 
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
    printf("Max patch vertices: %d\n", maxPatchVertices);
    glPatchParameteri(GL_PATCH_VERTICES, 3);

    check = LoadObj(object, vertices, UVs, normals, mtlFile);

	if (check == false) {
		OutputDebugStringA("Error, can not load object!\n");
	}

    check = LoadObj(object2, vertices2, UVs2, normals2, mtlFile2);

    if (check == false) {
        OutputDebugStringA("Error, can not load object!\n");
    }

    //creating the tangent for the normalmap
    vector<vec3> tangent = CreateTangent(vertices, UVs, normals);
    vector<vec3> tangent2 = CreateTangent(vertices2, UVs2, normals2);

    // Vertex Array Object (VAO), description of the inputs to the GPU 
    glGenVertexArrays(1, &gVertexAttribute);

    // bind is like "enabling" the object to use it
    glBindVertexArray(gVertexAttribute);

    // create a vertex buffer object (VBO) id (out Array of Structs on the GPU side)
    glGenBuffers(1, &gVertexBuffer);

    // Bind the buffer ID as an ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);

    // This "could" imply copying to the GPU, depending on what the driver wants to do, and
    // the last argument (read the docs!)
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &gUVbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, gUVbuffer);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(vec2), &UVs[0], GL_STATIC_DRAW);

    glGenBuffers(1, &gNormalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, gNormalbuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);

    glGenBuffers(1, &gTangentbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, gTangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, tangent.size() * sizeof(vec3), &tangent[0], GL_STATIC_DRAW);

    // query which "slot" corresponds to the input vertex_position in the Vertex Shader 
    GLint vertexPos = glGetAttribLocation(gShaderProgram, "vertex_position");
    // if this returns -1, it means there is a problem, and the program will likely crash.
    // examples: the name is different or missing in the shader

    if (vertexPos == -1) {
        OutputDebugStringA("Error, cannot find 'vertex_position' attribute in Vertex shader\n");
    }

    // this activates the first attribute of this VAO
    // think of "attributes" as inputs to the Vertex Shader
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
    glVertexAttribPointer(
        vertexPos,				// location in shader
        3,						// how many elements of type (see next argument)
        GL_FLOAT,				// type of each element
        GL_FALSE,				// integers will be normalized to [-1,1] or [0,1] when read...
        0,						// distance between two vertices in memory (stride)
        (void*)(0)				// offset of FIRST vertex in the list.
    );

    // repeat the process for the second attribute.
    GLuint vertexTex = glGetAttribLocation(gShaderProgram, "tex_coord");

    if (vertexTex == -1) {
        OutputDebugStringA("Error, cannot find 'tex_coord' attribute in Vertex shader\n");
    }

    // this activates the second attribute of this VAO
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, gUVbuffer);
    glVertexAttribPointer(
        vertexTex,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    // repeat the process for the third attribute.
    GLuint normalFaces = glGetAttribLocation(gShaderProgram, "normal_faces");

    if (normalFaces == -1) {
        OutputDebugStringA("Error, cannot find 'normal_faces' attribute in Vertex shader\n");
    }

    // this activates the third attribute of this VAO
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, gNormalbuffer);
    glVertexAttribPointer(
        normalFaces,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    // repeat the process for the fourth attribute.
    GLuint tangentCoords = glGetAttribLocation(gShaderProgram, "tangent_coord");

    if (tangentCoords == -1) {
        OutputDebugStringA("Error, cannot find 'tangent_Coords' attribute in Vertex shader\n");
    }
    // this activates the fourth attribute of this VAO
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, gTangentbuffer);
    glVertexAttribPointer(
        tangentCoords,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    glBindVertexArray(0);

    glGenVertexArrays(1, &gVertexAttribute2);
    glBindVertexArray(gVertexAttribute2);

    glGenBuffers(1, &gVertexBuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer2);
    glBufferData(GL_ARRAY_BUFFER, vertices2.size() * sizeof(vec3), &vertices2[0], GL_STATIC_DRAW);

    glGenBuffers(1, &gUVbuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, gUVbuffer2);
    glBufferData(GL_ARRAY_BUFFER, UVs2.size() * sizeof(vec2), &UVs2[0], GL_STATIC_DRAW);

    glGenBuffers(1, &gNormalbuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, gNormalbuffer2);
    glBufferData(GL_ARRAY_BUFFER, normals2.size() * sizeof(vec3), &normals2[0], GL_STATIC_DRAW);

    glGenBuffers(1, &gTangentbuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, gTangentbuffer2);
    glBufferData(GL_ARRAY_BUFFER, tangent2.size() * sizeof(vec3), &tangent2[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer2);
    glVertexAttribPointer(
        vertexPos,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, gUVbuffer2);
    glVertexAttribPointer(
        vertexTex,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, gNormalbuffer2);
    glVertexAttribPointer(
        normalFaces,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, gTangentbuffer2);
    glVertexAttribPointer(
        tangentCoords,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    glBindVertexArray(0);

    //För skuggan
    GLuint vertexPosShadow = glGetAttribLocation(shadowShader, "vertex_position_shadow");

    if (vertexPosShadow == -1) {
        OutputDebugStringA("Error, cannot find 'vertex_position_shadow' attribute in ShadowVS\n");
    }

    glGenVertexArrays(1, &gVertexAttributeShadow);
    glBindVertexArray(gVertexAttributeShadow);

    glGenBuffers(1, &gVertexBuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer2);
    glBufferData(GL_ARRAY_BUFFER, vertices2.size() * sizeof(vec3), &vertices2[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer2);
    glVertexAttribPointer(
        vertexPosShadow,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)(0)
    );

    glBindVertexArray(0);

    nrOfVertices_out = vertices.size();
    nrOfVertices2_out = vertices2.size();
	return vertices2;
}

void TextureCreator(int width, int height, unsigned char * textureData, GLuint * texture) {
    //generates the texture
    glGenTextures(1, texture);

    //binds the texture to the active texture unit
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D,
        0,									//mipmappinglevel (quality of distant textures)
        GL_RGBA,
        width, height,						//width, height of the texture
        0,
        GL_RGBA, GL_UNSIGNED_BYTE,			//format (colorformat) and type (datatype)
        textureData);						//data (texturedata)

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);
}

void Render(int nrOfVertices, GLint textureLoc, GLint normalMapLoc,
	GLint depthMapTexLoc, GLuint vertexAttribute, GLuint texture, GLuint normalMap)
{
    // tell opengl we are going to use the VAO we described earlier
    glBindVertexArray(vertexAttribute);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(textureLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap);
    glUniform1i(normalMapLoc, 1);

    // ask OpenGL to draw 3 vertices starting from index 0 in the vertex array 
    // currently bound (VAO), with current in-use shader. Use TOPOLOGY GL_TRIANGLES,
    // so for one triangle we need 3 vertices!
    glDrawArrays(GL_PATCHES, 0, nrOfVertices);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void configureGBuffer() {
	glGenFramebuffers(1, &GBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);

	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// color 
	glGenTextures(1, &gMaterial);
	glBindTexture(GL_TEXTURE_2D, gMaterial);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gMaterial, 0);

    // particle 
    glGenTextures(1, &gParticle);
    glBindTexture(GL_TEXTURE_2D, gParticle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gParticle, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOWWIDTH, WINDOWHEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		OutputDebugStringA("Framebuffer not complete!\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glUniform1i(glGetUniformLocation(lightShaderProgram, "gPosition"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glUniform1i(glGetUniformLocation(lightShaderProgram, "gNormal"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gMaterial);
	glUniform1i(glGetUniformLocation(lightShaderProgram, "gMaterial"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gParticle);
    glUniform1i(glGetUniformLocation(lightShaderProgram, "gParticle"), 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthMapTex);
    glUniform1i(depthMapTexLoc, 4);

    glUniform3fv(camPosLocLight, 1, camPos);

    //sending directional light attributes
    glUniform3fv(directionalLightLoc, 1, lightPos);
    glUniform3fv(lightColorLoc, 1, lightCol);
    glUniform3fv(lightAmbLoc, 1, lightAmb);
    glUniform3fv(lightDiffLoc, 1, lightDiff);
    glUniform3fv(lightSpecLoc, 1, lightSpec);
}

// shadowmapping - create fbo and texture for the depthmap
void createDepthMap()
{
    //GLuint depthMap;
    glGenTextures(1, &depthMapTex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthMapTex);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT,
        0,
        GL_DEPTH_COMPONENT, GL_FLOAT,
        NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        OutputDebugStringA("Framebuffer Error!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// shadowmapping
void renderToDepthMap(int nrOfVertices)
{
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    glBindVertexArray(gVertexAttributeShadow);

    glDrawArrays(GL_TRIANGLES, 0, nrOfVertices);
    glBindVertexArray(0);
    glViewport(0, 0, WINDOWWIDTH, WINDOWHEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// gauss - framebuffer to render scene to
void createSceneFBO()
{
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // depth
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOWWIDTH, WINDOWHEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    // attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

    GLenum drawBuffer[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//gauss pingpong buffers
void pingPongBuffers()
{
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGB, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
        );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// gauss - quad to render the blurred texture to
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// gauss - sends the scene-texture to the pingpongbuffers and blurs the texture horizontally/vertically every other pass
void gaussianBlur(int passes)
{
    if (passes == 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderQuad();
    }
    else
    {
        bool horizontal = true, first_iteration = true;
        int amount = passes * 2;
        glUseProgram(blurShaderProgram);
        glActiveTexture(GL_TEXTURE0);
        for (int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glUniform1i(glGetUniformLocation(blurShaderProgram, "horizontal"), horizontal);
            glBindTexture(
                GL_TEXTURE_2D, first_iteration ? sceneTexture : pingpongBuffer[!horizontal]
            );
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(quadShaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
        renderQuad();
    }
}

// BP Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
int FindUnusedParticle() {

    for (int i = LastUsedParticle; i < MAXPARTICLES; i++) {
        if (ParticlesContainer[i].life < 0) {
            LastUsedParticle = i;
            return i;
        }
    }

    for (int i = 0; i<LastUsedParticle; i++) {
        if (ParticlesContainer[i].life < 0) {
            LastUsedParticle = i;
            return i;
        }
    }

    return 0; // All particles are taken, override the first one
}

// BP
void CreateParticleData() {

    glGenVertexArrays(1, &gVertexAttributeParticle);
    glBindVertexArray(gVertexAttributeParticle);

    //Vertices of the particles
    static const GLfloat pBufferData[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };

    glGenBuffers(1, &billVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, billVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pBufferData), pBufferData, GL_STATIC_DRAW);

    //Positions and size of the particles
    glGenBuffers(1, &pPosBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pPosBuffer);
    //Buffer is initialized empty because it will be updated later per each frame
    glBufferData(GL_ARRAY_BUFFER, MAXPARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    //Colors of the particles
    glGenBuffers(1, &pColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAXPARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billVertexBuffer);
    glVertexAttribPointer(
        0,								   // attribute. No particular reason for 0, but must match the layout in the shader.
        3,								   // size
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
    );

    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, pPosBuffer);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size : x + y + z + size => 4
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
    );

    // 3rd attribute buffer : particles' colors
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, pColorBuffer);
    glVertexAttribPointer(
        2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size : r + g + b + a => 4
        GL_FLOAT,
        GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
        0,
        (void*)0
    );

    glBindVertexArray(0);
}

void SetViewport()
{
    // usually (not necessarily) this matches with the window size
    glViewport(0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT);
}

void MouseMovement() {
    if (GetKeyState(VK_LBUTTON) < 0) { //Runs if the left mousebutton is pressed
        GetCursorPos(&mousePos);
        cam.mouseUpdate(vec2(mousePos.x, mousePos.y));
    }
}

void KeyMovement() {
    if (GetKeyState('W') & 0x8000) {
        cam.forward();
    }
    if (GetKeyState('S') & 0x8000) {
        cam.backward();
    }
    if (GetKeyState('D') & 0x8000) {
        cam.right();
    }
    if (GetKeyState('A') & 0x8000) {
        cam.left();
    }
}

void deleteGlobalData() {
	glDeleteTextures(1, &gTexture);
	glDeleteTextures(1, &gTexture2);
	glDeleteTextures(1, &gNormalMap);
	glDeleteTextures(1, &gNormalMap2);
	glDeleteTextures(1, &depthMapTex);
	glDeleteTextures(1, &gParticleTexture);
	glDeleteTextures(1, &gPosition);
	glDeleteTextures(1, &gNormal);
	glDeleteTextures(1, &gMaterial);
	glDeleteTextures(1, &sceneTexture);

	glDeleteBuffers(1, &gVertexBuffer);
	glDeleteBuffers(1, &gUVbuffer);
	glDeleteBuffers(1, &gNormalbuffer);
	glDeleteBuffers(1, &gTangentbuffer);
	glDeleteBuffers(1, &gVertexBuffer2);
	glDeleteBuffers(1, &gUVbuffer2);
	glDeleteBuffers(1, &gNormalbuffer2);
	glDeleteBuffers(1, &gTangentbuffer2);
	glDeleteBuffers(1, &pColorBuffer);
	glDeleteBuffers(1, &pPosBuffer);
	glDeleteBuffers(1, &billVertexBuffer);
	glDeleteBuffers(1, &depthrenderbuffer);
	glDeleteBuffers(1, pingpongBuffer);
	glDeleteBuffers(1, &quadVBO);
	glDeleteBuffers(1, &vbo_vertices);
	glDeleteBuffers(1, &ibo_elements);

	glDeleteShader(vs);
	glDeleteShader(tcs);
	glDeleteShader(tes);
	glDeleteShader(gs);
	glDeleteShader(fs);
	glDeleteShader(shadowVs);
	glDeleteShader(shadowFs);
	glDeleteShader(lightVs);
	glDeleteShader(lightFs);
	glDeleteShader(particleVs);
	glDeleteShader(particleFs);
	glDeleteShader(blurVertexShader);
	glDeleteShader(blurFragShader);
	glDeleteShader(quadVertexShader);
	glDeleteShader(quadFragShader);
	glDeleteShader(BBOVs);
	glDeleteShader(BBOFs);

	glDeleteProgram(BBOProgram);
	glDeleteProgram(gShaderProgram);
	glDeleteProgram(shadowShader);
	glDeleteProgram(lightShaderProgram);
	glDeleteProgram(particleShader);
	glDeleteProgram(blurShaderProgram);
	glDeleteProgram(quadShaderProgram);

	glDeleteVertexArrays(1, &gVertexAttribute);
	glDeleteVertexArrays(1, &gVertexAttribute2);
	glDeleteVertexArrays(1, &gVertexAttributeShadow);
	glDeleteVertexArrays(1, &gVertexAttributeParticle);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteVertexArrays(1, &gVertexAttributeParticle);
	glDeleteVertexArrays(1, &gVertexAttributeBBO);

	glDeleteFramebuffers(1, &depthMapFBO);
	glDeleteFramebuffers(1, &sceneFBO);
	glDeleteFramebuffers(1, &GBuffer);
	glDeleteFramebuffers(1, pingpongFBO);

	glDeleteRenderbuffers(1, &rboDepth);

	glDeleteQueries(1, &query);
}

// BBO
mat4 configureBBO(vector<vec3> & in_vertices) {

	if (in_vertices.size() == 0) {
		OutputDebugStringA("ERROR! No vertices found!\n");
	}

	GLfloat min_x, max_x, min_y, max_y, min_z, max_z;
	min_x = max_x = in_vertices[0].x;
	min_y = max_y = in_vertices[0].y;
	min_z = max_z = in_vertices[0].z;

	for (unsigned int i = 0; i < in_vertices.size(); i++) {
		if (in_vertices[i].x < min_x) min_x = in_vertices[i].x;
		if (in_vertices[i].x > max_x) max_x = in_vertices[i].x;
		if (in_vertices[i].y < min_y) min_y = in_vertices[i].y;
		if (in_vertices[i].y > max_y) max_y = in_vertices[i].y;
		if (in_vertices[i].z < min_z) min_z = in_vertices[i].z;
		if (in_vertices[i].z > max_z) max_z = in_vertices[i].z;
	}

	// We compute the size necessary for the object : max - min in X, Y and Z
	vec3 size = vec3(max_x - min_x, max_y - min_y, max_z - min_z);
	// We compute the center of the object : (min + max) / 2 in X, Y and Z
	vec3 center = vec3((min_x + max_x) / 2, (min_y + max_y) / 2, (min_z + max_z) / 2);
	mat4 transform = translate(mat4(1), center) * scale(mat4(1), size);

	// We prepare a cube of size 1 (1x1x1), centered on the origin
	// Cube 1x1x1, centered on origin
	GLfloat cube[] = {
		 -0.5, -0.5, -0.5, 1.0,
		  0.5, -0.5, -0.5, 1.0,
		  0.5,  0.5, -0.5, 1.0,
		 -0.5,  0.5, -0.5, 1.0,
		 -0.5, -0.5,  0.5, 1.0,
		  0.5, -0.5,  0.5, 1.0,
		  0.5,  0.5,  0.5, 1.0,
		 -0.5,  0.5,  0.5, 1.0,
	};

	glGenVertexArrays(1, &gVertexAttributeBBO);
	glGenBuffers(1, &vbo_vertices);
	glBindVertexArray(gVertexAttributeBBO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), &cube[0], GL_STATIC_DRAW);

	GLushort elements[] = {
	  0, 1, 2, 3,
	  4, 5, 6, 7,
	  0, 4, 1, 5, 2, 6, 3, 7,
	};
	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), &elements[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return transform;
}

void draw_bbox(mat4 world, GLuint worldLoc, mat4 view, GLuint viewLoc, mat4 proj, GLuint projLoc) {

	glUniformMatrix4fv(worldLoc, 1, GL_FALSE, &world[0][0]);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);

	glBindVertexArray(gVertexAttributeBBO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	// We draw the front face using 1 looping line
	glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, 0);
	// We draw the back face using 1 looping line
	glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(4 * sizeof(GLushort)));
	// We draw 4 orthogonal lines to join both faces
	glDrawElements(GL_LINES, 8, GL_UNSIGNED_SHORT, (GLvoid*)(8 * sizeof(GLushort)));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
