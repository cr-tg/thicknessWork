#ifndef GLLIB
#define GLLIB
#include <iostream>

#ifdef	LIB_EXPORTS
#define LIB_API __declspec(dllexport) //dll项目中是导出，其它项目中代表导入
#else
#define LIB_API __declspec(dllimport) 
#endif


LIB_API void LibFunc1();


LIB_API void LibFunc2();
#endif // !GLLIB

