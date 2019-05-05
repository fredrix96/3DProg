#include "camera.h"
#include <glm\gtx\transform.hpp>
#include <GLFW\glfw3.h>

const float rotSpeed = 0.01f;
const float moveSpeed = 0.01f;

Camera::Camera():
	viewDir(0.0f, -1.0f, -1.0f),
	up(0.0f, 1.0f, 0.0f),
	position(0.0f, 3.0f, 3.0f){
}

Camera::~Camera() {
}

void Camera::mouseUpdate(const vec2 & newMousePos) {
	vec2 mouseDist = newMousePos - oldMousePos;

	//Makes sure that the camera does not jump when the newMousePos
	//is far away from the oldMousePos
	if (length(mouseDist) > 50.0f) {
		oldMousePos = newMousePos;
	}

	else if (newMousePos != oldMousePos) {
		//Get the perpendicular axis to the up- and viewDir-axis
		vec3 yAxis = cross(viewDir, up);

		mat4 rotator = rotate(-mouseDist.x * rotSpeed, up)
			* rotate(-mouseDist.y * rotSpeed, yAxis);

		viewDir = mat3(rotator) * viewDir;
		oldMousePos = newMousePos;
	}
}

void Camera::forward() {
	position += moveSpeed * viewDir;
}
void Camera::backward() {
	position += -moveSpeed * viewDir;
}
void Camera::right() {
	vec3 yAxis = cross(viewDir, up);
	position += moveSpeed * yAxis;
}
void Camera::left() {
	vec3 yAxis = cross(viewDir, up);
	position += -moveSpeed * yAxis;
}

vec3 Camera::getCamPos() const {
	return position;
}

mat4 Camera::getViewMatrix() const {
	return lookAt(position, position + viewDir, up);
}
