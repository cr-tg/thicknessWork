#include "Window.h"

CWindow* CWindow::m_pWindow;
int CWindow::m_ScrHeight = 0;
int CWindow::m_ScrWidth = 0;
std::string CWindow::m_WindowName;

void CWindow::setWindowSize(glm::uvec2 size)
{
	m_ScrWidth = size.x;
	m_ScrHeight = size.y;
}

void CWindow::setWindowName(const std::string& name)
{
	m_WindowName = name;
}

CWindow::CWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	m_pGlfwWindow = glfwCreateWindow(m_ScrWidth, m_ScrHeight, m_WindowName.c_str(), NULL, NULL);
	if (m_pGlfwWindow == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}
	glfwMakeContextCurrent(m_pGlfwWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD in dll" << std::endl;
		glfwTerminate();
	}
	else
	{
		std::cout << "successed to initialize GLAD in dll" << std::endl;
	}
}
