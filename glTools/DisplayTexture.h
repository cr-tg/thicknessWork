#pragma once
#include "setting.h"
#include <stb_image.h>
#include <learnopengl/shader.h>
#include <string>


//想要display出texture，就必须调用glad相关的函数，那么就必须初始化glad，如果想要初始化glad，必须创建一个窗口
//而且将上下文环境设置为这个窗口

class CTextureDisplay
{
public:
	static GLTOOLSDLL_API CTextureDisplay* getOrCreateWindowInstance()
	{
		if (m_pTextureDisplay == nullptr)
		{
			m_pTextureDisplay = new CTextureDisplay();
		}
		return m_pTextureDisplay;
	}

	GLTOOLSDLL_API void DisplayFramebufferTexture(GLuint textureID);

	GLTOOLSDLL_API void DisplayFramebufferTextureArray(GLuint textureID, GLuint vLayer = 0);

	GLTOOLSDLL_API void DisplayFramebufferTextureMsaa(GLuint textureID);

	GLTOOLSDLL_API unsigned int loadTexture(char const* path);


	GLTOOLSDLL_API CTextureDisplay* getTextureDisplay() const
	{
		return m_pTextureDisplay;
	}
private:

	static CTextureDisplay* m_pTextureDisplay;
	CTextureDisplay() 
	{
		//const std::string pathstr = "./shaders/";//dll加载到项目以后，使用的就是exe的当前路径了，因此这里要使用绝对路径
		const std::string pathstr = "E:/vs project/thickNessWork/thicknessWork/glTools/shaders/";
		texShowShader = Shader((pathstr + "texShow.vs").c_str(), (pathstr + "texShow.fs").c_str());
		texArrayShowShader = Shader((pathstr + "texShow.vs").c_str(), (pathstr + "texArrayShow.fs").c_str());
		texMsaaShowShader = Shader((pathstr + "texShow.vs").c_str(), (pathstr + "msaaTexShow.fs").c_str());
	}
	unsigned int texShowVAO = 0;
	unsigned int texShowVBO = 0;

	void __initTexShowVAO();
	Shader texShowShader;
	Shader texArrayShowShader;
	Shader texMsaaShowShader;
};



