#include "gladlsetting.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Window.h"
#include "TestStatic.h"
#include "DisplayTexture.h" 

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void callGLFromDll();
void callGLFromMain();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void error_callback(int error, const char* description);

using namespace std;

int main()
{
    callGLFromDll();
    //callGLFromMain();
    return 0;
}

void callGLFromMain()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    std::string s = "LearnOpenGL";
    GLFWwindow* ppwindow = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, s.c_str(), NULL, NULL);
    glfwMakeContextCurrent(ppwindow);
    if (ppwindow == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return;
    }
    else
    {
        std::cout << "successed to initialize GLAD in main" << std::endl;
    }
    std::cout << "main:" << glGetString(GL_VERSION) << std::endl;
    glfwSetFramebufferSizeCallback(ppwindow, framebuffer_size_callback);
    glfwSetWindowShouldClose(ppwindow, true);

    while (!glfwWindowShouldClose(ppwindow))
    {
        processInput(ppwindow);
        glfwSwapBuffers(ppwindow);
        glfwPollEvents();
    }
    glfwTerminate();
}

void callGLFromDll()
{
    CWindow::setWindowName("testDll");
    CWindow::setWindowSize(glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
    GLFWwindow* window = CWindow::getOrCreateWindowInstance()->getGlfwWindow();
    std::cout << CTextureDisplay::getVersion() << std::endl;
    std::cout << "main:" << glGetString(GL_VERSION) << std::endl;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetWindowShouldClose(window, true);

    while (!glfwWindowShouldClose(window))
    {
        //cout << "dll loop" << endl;
        processInput(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void error_callback(int error, const char* description)
{
    std::cout << "errorInx:" << error << ",message:" << description << std::endl;
}