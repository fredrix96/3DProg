#ifndef CAMERA_H
#define CAMERA_H
#include <glm\glm.hpp>

using namespace std;
using namespace glm;

class Camera {
private:
	vec3 position;
	vec3 viewDir;
	const vec3 up;
	vec2 oldMousePos; //default position will be at the centre of the screen
public:
	Camera();
	~Camera();
	void mouseUpdate(const vec2 & newMousePos); //the screenspace is 2D
	void forward();
	void backward();
	void right();
	void left();
	vec3 getCamPos() const;
	mat4 getViewMatrix() const;
};

#endif //CAMERA_H

