#pragma once
#include "setting.h"
//#include "glLib.h"

class CWindow
{
public:
	static GLTOOLSDLL_API CWindow* getOrCreateWindowInstance()
	{
		if (m_pWindow == nullptr)
		{
			m_pWindow = new CWindow();
		}
		return m_pWindow;
	}
	static GLTOOLSDLL_API void setWindowSize(glm::uvec2 size);
	static GLTOOLSDLL_API void setWindowName(const std::string& name);
	GLTOOLSDLL_API std::string getWindowName() const
	{
		return m_WindowName;
	}

	GLTOOLSDLL_API GLFWwindow* getGlfwWindow() const
	{
		return m_pGlfwWindow;
	}

private:
	CWindow();
	static CWindow* m_pWindow;
	static int m_ScrWidth;
	static int m_ScrHeight;
	static std::string m_WindowName;

	GLFWwindow* m_pGlfwWindow;
};

