#pragma once
#include "setting.h"
#include <stb_image.h>
#include <learnopengl/shader.h>
#include <string>


//��Ҫdisplay��texture���ͱ������glad��صĺ�������ô�ͱ����ʼ��glad�������Ҫ��ʼ��glad�����봴��һ������
//���ҽ������Ļ�������Ϊ�������

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
		//const std::string pathstr = "./shaders/";//dll���ص���Ŀ�Ժ�ʹ�õľ���exe�ĵ�ǰ·���ˣ��������Ҫʹ�þ���·��
		const std::string pathstr = "E:/vs project/thicknessWork/glTools/shaders/";
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



