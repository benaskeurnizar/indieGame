#ifndef SHADERS_H
#define SHADERS_H

#include <stdlib.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <windows.h>


char* LoadShaderText(char* address,int* size);

void LinkVShader(unsigned int*VertexShader,const char* VertexCodeAdr);
void LinkFrShader(unsigned int*FragmentShader,const char* FragmentCodeAdr);
void LinkPShader(unsigned int* VertexShader,unsigned int* FragmentShader,unsigned int* ProgramShader);

#endif
