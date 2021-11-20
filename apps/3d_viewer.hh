// header file for 3d_viewer.cpp
#ifndef THREE_D_VIEWER_FUNCS
#define THREE_D_VIEWER_FUNCS

#include "../utils/Camera.hh"
#include "imgui.h"

#include <chrono>
#include <iostream>
#include <math.h>
#include <vector>
#include <GL/glew.h>
#include <GLM/vec2.hpp>
#include <GLM/vec3.hpp>
#include <GLM/mat4x4.hpp>
#include <GLM/ext.hpp>

float angleToRads(float angleInDeg);

glm::mat4 createPerspectiveMatrix(float fovAngle, float aspect, float near, float far);
glm::mat4 lookAt(const glm::vec3& from, const glm::vec3& to, const glm::vec3& tmp);

void getObjModelInfo(
	auto& shapes, 
	auto& attrib, 
	std::vector<glm::vec3>& vertices,
	std::vector<glm::vec2>& uvs,
	std::vector<glm::vec3>& normals
);

void setupShaders(GLuint& shaderProgram);

void setupOffscreenFramebuffer(GLuint* frameBuffer, GLuint* texture);

void show3dModelViewer(
	GLuint offscreenFrameBuf, 
	GLuint offscreenTexture, 
	GLuint shaderProgram,
	GLuint vbo,
	GLuint vao,
	GLuint uvBuffer,
	GLuint matrixId,
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime
);



#endif