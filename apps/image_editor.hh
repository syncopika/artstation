// header file for image_editor.cpp
#ifndef IMAGE_EDITOR_FUNCS
#define IMAGE_EDITOR_FUNCS

#include "imgui.h"

#include <string>
#include <GL/glew.h>

std::string trimString(std::string str);
int correctRGB(int channel);
bool importImage(const char* filename, GLuint* tex, GLuint* originalImage, int* width, int* height);
void showImageEditor();

#endif