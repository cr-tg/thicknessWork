#pragma once
#include "gladlsetting.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>


#ifdef	GLTOOLS_EXPORTS
#define GLTOOLSDLL_API __declspec(dllexport)//dll��Ŀ���ǵ�����������Ŀ�д�����
#else
#define GLTOOLSDLL_API __declspec(dllimport) 
#endif