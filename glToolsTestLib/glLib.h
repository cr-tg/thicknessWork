#ifndef GLLIB
#define GLLIB
#include <iostream>

#ifdef	LIB_EXPORTS
#define LIB_API __declspec(dllexport) //dll��Ŀ���ǵ�����������Ŀ�д�����
#else
#define LIB_API __declspec(dllimport) 
#endif


LIB_API void LibFunc1();


LIB_API void LibFunc2();
#endif // !GLLIB

