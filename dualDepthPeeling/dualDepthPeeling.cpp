#include "DisplayTexture.h"
#include "Window.h"
#include "Render.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <map>
#include <tuple>

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

#define allDepthLayer 6

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 4.5f));//6.0f
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 targetPos(0.0f, 0.0f, 0.0f);//测试view矩阵的up向量的初始值

unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int lightProjVAO;
unsigned int lightProjVBO;

unsigned int pingPongFrameBuffer[2];
unsigned int pgFrontAndBackDepth[2];
unsigned int pgDepthComponent[2];
unsigned int frontTex;
unsigned int backTex;
vector<GLuint> attachments = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 };
GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };


void setUpFramebuffers();
void renderTransparentScene(CRender* pRender,const Shader& vShader);


int main()
{
	//init context,createWindow and make it currentContext
	CWindow::setWindowName("dual_depth_peeling");
	CWindow::setWindowSize(glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
	GLFWwindow* window = CWindow::getOrCreateWindowInstance()->getGlfwWindow();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//设置render单例，用来绘制各种简单的形状，比如单层图形，cube，triangle,plane等
	CRender* dllRender = CRender::getOrCreateWindowInstance()->getRender();

	//设置displayTexture函数，用来展示各种texture，主要是用来调试的,而且因为它一定是采样texture，然后show出来
	//因此可以将shader也在函数内部里面设置
	CTextureDisplay* dllTextureDisplay = CTextureDisplay::getOrCreateWindowInstance()->getTextureDisplay();

	unsigned int woodTexture = dllTextureDisplay->loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	setUpFramebuffers();

	//定义shader
	const std::string pathstr = "./shaders/";
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader dualDepthInitShader((pathstr + "dualDepthInit.vs").c_str(), (pathstr + "dualDepthInit.fs").c_str());
	Shader dualDepthPeelingShader((pathstr + "dualDepthPeeling.vs").c_str(), (pathstr + "dualDepthPeeling.fs").c_str());
	Shader blendShader((pathstr + "blend.vs").c_str(), (pathstr + "blend.fs").c_str());

	float camera_nearPlane = 0.1f, camera_farPlane = 10.0f;
	quadShader.use();
	quadShader.setInt("uDepthNum", allDepthLayer);
	//绘制透明物体的时候一定要排序，1.绘制所有的不透明物体。2.将透明的物体排序，再从远到近的渲染
	//因为深度测试的原因，如果不按照从远到近的顺序绘制的话，因为深度写入的原因，近的透明的物体
	//的深度会写入到深度缓冲中，然后远处的在这个透明物体的后面的透明物体就会因为深度测试被丢弃
	//因此只能从远到近的绘制
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		const uint32_t front_clear[4] = { 0, 0, 0, 1 };
		const uint32_t back_clear[4] = { 0, 0, 0, 0 };
		const float depth_clear[4] = { -1.0, -1.0, -1.0, -1.0 };

		glBindFramebuffer(GL_FRAMEBUFFER, pingPongFrameBuffer[0]);
		glClearBufferfv(GL_COLOR, 0, depth_clear);
		glClearBufferuiv(GL_COLOR, 1, front_clear);
		glClearBufferuiv(GL_COLOR, 2, back_clear);

		glBindFramebuffer(GL_FRAMEBUFFER, pingPongFrameBuffer[1]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearBufferfv(GL_COLOR, 0, depth_clear);
		glClearBufferuiv(GL_COLOR, 1, front_clear);
		glClearBufferuiv(GL_COLOR, 2, back_clear);
		dualDepthInitShader.use();
		dualDepthInitShader.setMat4("projection", cameraProjection);
		dualDepthInitShader.setMat4("view", cameraView);

		dualDepthInitShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
		dllRender->renderPlane(dualDepthInitShader);
		renderTransparentScene(dllRender, dualDepthInitShader);

		for (int layer = 0; layer < 1; ++layer)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingPongFrameBuffer[layer % 2]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearBufferfv(GL_COLOR, 0, depth_clear);
			dualDepthPeelingShader.use();
			dualDepthPeelingShader.setMat4("projection", cameraProjection);
			dualDepthPeelingShader.setMat4("view", cameraView);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pgFrontAndBackDepth[(layer + 1) % 2]);
			dualDepthPeelingShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
			dllRender->renderPlane(dualDepthPeelingShader);
			renderTransparentScene(dllRender, dualDepthPeelingShader);
		}

		//可视化
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		blendShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, frontTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, backTex);
		dllRender->renderQuad();

		//quadShader.use();
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, frontTex);
		//dllRender->renderQuad();
		//dllTextureDisplay->DisplayFramebufferTexture(pgFrontAndBackDepth[1]);
		//dllTextureDisplay->DisplayFramebufferTexture(pgFrontAndBackDepth[0]);
		//dllTextureDisplay->DisplayFramebufferTexture(frontTex);
		//dllTextureDisplay->DisplayFramebufferTexture(backTex);
		glEnable(GL_DEPTH_TEST);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &quadVBO);

	glfwTerminate();
	return 0;
}


void setUpFramebuffers()
{
	glGenFramebuffers(2, pingPongFrameBuffer);
	glGenTextures(2, pgFrontAndBackDepth);
	glGenTextures(2, pgDepthComponent);
	glGenTextures(1, &frontTex);
	glGenTextures(1, &backTex);

	glBindTexture(GL_TEXTURE_2D, frontTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//光源看不到的地方的深度为1.0
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D, backTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//光源看不到的地方的深度为1.0
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);

	for (int i = 0; i < 2; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, pgFrontAndBackDepth[i]);
																		   //这里只能是GL_RG
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RG,
					 GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, pgDepthComponent[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT,
			GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindTexture(GL_TEXTURE_2D, 0);
		
	}

	for (int i = 0; i < 2; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingPongFrameBuffer[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pgFrontAndBackDepth[i], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, frontTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, backTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pgDepthComponent[i], 0);
		glDrawBuffers(3, attachments.data());

		glBlendFuncSeparatei(0, GL_ONE, GL_ONE, GL_ONE, GL_ONE);
		glBlendFuncSeparatei(1, GL_DST_ALPHA, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
													//GL_ONE, GL_ZERO
		glBlendFuncSeparatei(2, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);
		glBlendEquationi(1, GL_FUNC_ADD);
		glBlendEquationi(2, GL_FUNC_ADD);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << i << "_FBO not complete！" << endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void renderTransparentScene(CRender* pRender, const Shader& vShader)
{
	glDepthMask(GL_FALSE);
	//pRender->renderSingFace(vShader, glm::vec3(0.0f, 0.2f, 4.0f), glm::vec4(0.0, 1.0, 0.0, 0.5));
	//pRender->renderSingFace(dualDepthInitShader, glm::vec3(0.0f, 0.2f, 3.5f), glm::vec4(1.0, 0.0, 0.0, 0.4));
	pRender->renderSingFace(vShader, glm::vec3(0.0f, 0.2f, 3.0f), glm::vec4(0.0, 0.0, 1.0, 0.3));
	//pRender->renderSingFace(dualDepthInitShader, glm::vec3(0.0f, 0.2f, 2.5f), glm::vec4(1.0, 1.0, 0.0, 0.2));
	//pRender->renderSingFace(dualDepthInitShader, glm::vec3(0.0f, 0.2f, 2.0f), glm::vec4(1.0, 0.0, 1.0, 0.1));
	pRender->renderSingFace(vShader, glm::vec3(0.0f, 0.2f, 1.5f), glm::vec4(0.0, 1.0, 1.0, 0.6));
	glDepthMask(GL_TRUE);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
	{
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE)
	{
	}
}



// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
