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

#define listLayer 10
#define lightColor glm::vec4(255,255,255,255)/glm::vec4(255)
#define virtualLightColor glm::vec4(226,204,239,255)/glm::vec4(255)
#define lightRadius 0.04f

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 736;

//计算着色器的全局工作组大小
const struct
{
	GLuint num_groups_x;
	GLuint num_groups_y;
	GLuint num_groups_z;
}dispatch_params = { 40,23,1 };

// camera
Camera camera(glm::vec3(0.0f, 0.2f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int lightProjVAO;
unsigned int lightProjVBO;

int main()
{
	//init context,createWindow and make it currentContext
	CWindow::setWindowName("SDF_THICK");
	CWindow::setWindowSize(glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
	GLFWwindow* window = CWindow::getOrCreateWindowInstance()->getGlfwWindow();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetWindowPos(window, 100, 100);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//设置render单例，用来绘制各种简单的形状，比如单层图形，cube，triangle,plane等
	CRender* dllRender = CRender::getOrCreateWindowInstance()->getRender();

	//设置displayTexture函数，用来展示各种texture，主要是用来调试的,而且因为它一定是采样texture，然后show出来
	//因此可以将shader也在函数内部里面设置
	CTextureDisplay* dllTextureDisplay = CTextureDisplay::getOrCreateWindowInstance()->getTextureDisplay();


	unsigned int woodTexture = dllTextureDisplay->loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());
	glEnable(GL_DEPTH_TEST);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };



	//定义shader
	const std::string pathstr = "./shaders/";
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader linkListCamShader((pathstr + "link_list_cam.vs").c_str(), (pathstr + "link_list_cam.fs").c_str());
	Shader linkListSampleSortShader((pathstr + "sortAndBlend.vs").c_str(), (pathstr + "sortAndBlend.fs").c_str());
	Shader lightShader((pathstr + "renderLight.vs").c_str(), (pathstr + "renderLight.fs").c_str());
	Shader simpleDepthShader((pathstr + "simpleShadow.vs").c_str(), (pathstr + "simpleShadow.fs").c_str());
	Shader sdfThickShader((pathstr + "sdfThick.vs").c_str(), (pathstr + "sdfThick.fs").c_str());
	Shader gBufferShader((pathstr + "gBuffer.vs").c_str(), (pathstr + "gBuffer.fs").c_str());


	linkListCamShader.use();
	GLuint64 maxListNodeNum = SCR_WIDTH * SCR_HEIGHT * listLayer;
	GLuint64 nodeSize = sizeof(glm::uvec4);
	GLuint64 ssboCamNodeSize = sizeof(glm::uvec3);
	GLuint64 ssboLightNodeSize = sizeof(uint32_t) + sizeof(uint32_t);
	linkListCamShader.setInt("uMaxListNode", maxListNodeNum);

	glm::vec3 lightPos(0.0f, 0.2f, -0.1f);
	//glm::vec3 lightPos(-2.0f, 1.0f, -1.0f);
	glm::vec3 virLightPos(0.0f, 0.2f, 5.5f);//5.0时下方有一排深度为180左右的点，可能是相切的点
	glm::mat4 lightProjection, lightView;
	float light_nearPlane = 0.1f, light_farPlane = 5.0f;
	glm::vec3 lightFront(0.0, 0.0, 1.0f);
	lightProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, light_nearPlane, light_farPlane);

	unsigned int singleColorFBO;
	glGenFramebuffers(1, &singleColorFBO);

	unsigned int singleTex;
	glGenTextures(1, &singleTex);
	glBindTexture(GL_TEXTURE_2D, singleTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	unsigned int singleDepthMap;
	glGenTextures(1, &singleDepthMap);
	glBindTexture(GL_TEXTURE_2D, singleDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, singleColorFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, singleTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, singleDepthMap, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//真实光源下的最近的深度
	unsigned int  realLightDepthMap;
	glGenTextures(1, &realLightDepthMap);
	glBindTexture(GL_TEXTURE_2D, realLightDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, realLightDepthMap, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//缓冲用来保存原子计数器，这个原子计数器给了每个片段唯一的序号标识
	unsigned int atomicBuffer;
	glGenBuffers(1, &atomicBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint64), nullptr, GL_DYNAMIC_COPY);
	const int one = 1;
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint64), &one);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	unsigned int headPtrImage;//头节点表
	glGenTextures(1, &headPtrImage);
	glBindTexture(GL_TEXTURE_2D, headPtrImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);//这里的level只能填1
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(1, headPtrImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);//绑定为image

	unsigned int lightHeadPtrImage;//这个是在光源视角下的头结点表
	glGenTextures(1, &lightHeadPtrImage);
	glBindTexture(GL_TEXTURE_2D, lightHeadPtrImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);//这里的level只能填1
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(2, lightHeadPtrImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);//绑定为image

	//使用PBO初始化头结点表
	GLuint headPtrInitPBO;
	glGenBuffers(1, &headPtrInitPBO);
	glBindBuffer(GL_ARRAY_BUFFER, headPtrInitPBO);
	glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_R32UI), NULL, GL_STATIC_DRAW);
	void* initData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memset(initData, 0, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_R32UI));
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	unsigned int thickNessImage;//保存thick的表，和深度图类似，深度图存储的是最近点的深度，这个存储的是最近点到最后面的点的厚度
	glGenTextures(1, &thickNessImage);
	glBindTexture(GL_TEXTURE_2D, thickNessImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(3, thickNessImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	unsigned int ssboCam;//存储所有片段的ssboCam，并且链接到着色器的0号位置
	glGenBuffers(1, &ssboCam);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCam);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxListNodeNum * ssboCamNodeSize, NULL, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboCam);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	unsigned int ssboLight;
	glGenBuffers(1, &ssboLight);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboLight);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxListNodeNum * ssboLightNodeSize, NULL, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboLight);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	//存储相机下看到的最近的片段的世界坐标，即着色点
	unsigned int gPositionTex;
	glGenTextures(1, &gPositionTex);
	glBindTexture(GL_TEXTURE_2D, gPositionTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned int gRboDepth;
	glGenRenderbuffers(1, &gRboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, gRboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);

	unsigned int gBufferFBO;
	glGenFramebuffers(1, &gBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	float camera_nearPlane = 0.01f, camera_farPlane = 15.0f;
	quadShader.use();
	quadShader.setFloat("near_plane", light_nearPlane);
	quadShader.setFloat("far_plane", light_farPlane);
	quadShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
	linkListSampleSortShader.use();
	linkListSampleSortShader.setFloat("far_plane", light_farPlane);
	linkListSampleSortShader.setFloat("near_plane", light_nearPlane);
	linkListSampleSortShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
	glm::vec3 staticCameraPos1 = glm::vec3(3.2, 0.2, 2.0);
	glm::vec3 staticCameraFront1 = glm::vec3(-1.0, 0.0, 0.0);
	glm::mat4 staticCameraView1 = glm::lookAt(staticCameraPos1, staticCameraPos1 + staticCameraFront1, glm::vec3(0.0, 1.0, 0.0));
	//正面的摄像机视点
	glm::vec3 staticCameraPos2 = glm::vec3(0.0, 0.2, 6.0);
	glm::vec3 staticCameraFront2 = glm::vec3(0.0, 0.0, -1.0);
	glm::mat4 staticCameraView2 = glm::lookAt(staticCameraPos2, staticCameraPos2 + staticCameraFront2, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 staticCameraProjection2 = glm::perspective(glm::radians(12.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);

	while (!glfwWindowShouldClose(window))
	{
		//初始化原子计数器为1
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint64), &one);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		//初始化头节点image的数据为0
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, headPtrInitPBO);
		glBindTexture(GL_TEXTURE_2D, headPtrImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, lightHeadPtrImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, thickNessImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);


		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);

		lightView = glm::lookAt(lightPos, lightPos+lightFront, glm::vec3(0.0, 1.0, 0.0));
		glClearColor(0.10f, 0.10f, 0.10f, 0.0f);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);//这样写比上面的可读性高一点
		//真实光源下的shadow map
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		simpleDepthShader.use();
		simpleDepthShader.setMat4("projection", lightProjection);
		simpleDepthShader.setMat4("view", lightView);
	//	dllRender->renderCubeSetFromData(simpleDepthShader, cubeSceneData);
		dllRender->renderSphereSet(simpleDepthShader);

		//初始化原子计数器为1
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint64), &one);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		//真实光源下的OIT,计算camera下的颜色
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		linkListCamShader.use();
		linkListCamShader.setMat4("projection", cameraProjection); 
		linkListCamShader.setMat4("view", cameraView);
		//不透明的物体，写入到深度缓冲中
		linkListCamShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
		//dllRender->renderPlane(linkListCamShader);
		glDepthMask(GL_FALSE);
		//dllRender->renderCubeSetFromData(linkListCamShader, cubeSceneData);
		dllRender->renderSphereSet(linkListCamShader);
		glDepthMask(GL_TRUE);
		//glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);//可以让等待的区域更小
		//排序,并且计算出最后的颜色的pass
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		linkListSampleSortShader.use();
		dllRender->renderQuad();

		//gBuffer的pass
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gBufferShader.use();
		//gBufferShader.setMat4("view", staticCameraView2);
		//gBufferShader.setMat4("projection", staticCameraProjection2);
		gBufferShader.setMat4("view", staticCameraView1);
		gBufferShader.setMat4("projection", cameraProjection);
		//gBufferShader.setMat4("view", cameraView);
		dllRender->renderSphereSet(gBufferShader);

		//从着色点到真实光源下的阴影点形成的线段进行采样并且根据虚拟光源的OIT进行厚度生成的pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sdfThickShader.use();
		sdfThickShader.setMat4("realLightProjection", lightProjection);
		sdfThickShader.setMat4("realLightView", lightView);
		sdfThickShader.setFloat("real_near_plane", light_nearPlane);
		sdfThickShader.setFloat("real_far_plane", light_farPlane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, realLightDepthMap);
		dllRender->renderQuad();

		//可视化
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quadShader.use();
		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, gPositionTex);
		//glBindTexture(GL_TEXTURE_2D, realLightDepthMap);
		glBindTexture(GL_TEXTURE_2D, singleTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, realLightDepthMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, headPtrImage);
		dllRender->renderQuad();
		//在最后添加对光源的渲染是OK的,因为是最后渲染的，所以肯定不会被前面的物体遮挡
		//这里有个方法可以让它不会挡住其它物体，就是在绘制其它物体是保留深度信息，然后使用glBlit将深度信息复制到默认帧缓冲中
		lightShader.use();
		lightShader.setMat4("projection", cameraProjection);
		lightShader.setMat4("view", staticCameraView1);
		//lightShader.setMat4("view", cameraView);
		dllRender->renderSphere(lightShader, lightPos, glm::vec3(lightRadius), lightColor);
		dllRender->renderSphere(lightShader, virLightPos, glm::vec3(lightRadius), virtualLightColor);
		glEnable(GL_DEPTH_TEST);
		cout << camera.Zoom << endl;
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
