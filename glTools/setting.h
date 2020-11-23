#pragma once
#include "gladlsetting.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>


#ifdef	GLTOOLS_EXPORTS
#define GLTOOLSDLL_API __declspec(dllexport)//dll项目中是导出，其它项目中代表导入
#else
#define GLTOOLSDLL_API __declspec(dllimport) 
#endif