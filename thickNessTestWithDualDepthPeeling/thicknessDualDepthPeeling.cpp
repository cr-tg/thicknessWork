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

#define allDepthLayer 2
#define lightColor glm::vec4(0.0,0.0,255,255)/glm::vec4(255)
#define lightRadius 0.4f

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 736;
glm::vec2 windowPos(500, 100);

//计算着色器的全局工作组大小
const struct
{
	GLuint num_groups_x;
	GLuint num_groups_y;
	GLuint num_groups_z;
}dispatch_params = { 40,23,1 };

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//顶点数据
unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int lightProjVAO;
unsigned int lightProjVBO;

GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat depthClear[] = { -1.0, -1.0, -1.0, -1.0 };

//gbuffer中的数据
unsigned int gPositionTex;
unsigned int gRboDepth;
unsigned int gBufferFBO;
void setGBuffer();

//每一pass的最后进行一次复制确实很傻，因此还是使用pingpongbuffer的解决方案

//每次剥离，将这一层的深度和颜色保存到这一次的结果中，然后再将深度复制到allDepthMap的对应层中，颜色复制到allColorFBO中

unsigned int pingpongFBO[2];
unsigned int frontAndBackDepth[2];
unsigned int depthMap[2];
void setFrameBuffers();

unsigned int allFrontAndBackDepthMap;//保存所有层的前面与后面深度的纹理数组
void setAllDepthMap();

//保存thick的表，和深度图类似，深度图存储的是最近点的深度，这个存储的是最近点到最后面的点的厚度
unsigned int thickNessImage;
unsigned int sdfThickImage;//存储的sdf的深度，用来做比较的
unsigned int errorImage;//存储误差的
unsigned int errorFBO;//因为误差image是通过compute shader来计算的，因此需要一个fbo来作为输出
void setImages();

GLuint headPtrInitPBO;//使用PBO初始化thick表,pbo中的值全部为0
void setPbo();

void initImage();//每一帧开始阶段先初始化所有image


int main()
{
	//init context,createWindow and make it currentContext
	CWindow::setWindowName("dual_depth_peeling_thick");
	CWindow::setWindowSize(glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
	GLFWwindow* window = CWindow::getOrCreateWindowInstance()->getGlfwWindow();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowPos(window, windowPos.x, windowPos.y);

	//设置render单例，用来绘制各种简单的形状，比如单层图形，cube，triangle,plane等
	CRender* dllRender = CRender::getOrCreateWindowInstance()->getRender();
	CTextureDisplay* dllTextureDisplay = CTextureDisplay::getOrCreateWindowInstance()->getTextureDisplay();
	unsigned int woodTexture = dllTextureDisplay->loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());
	
	glm::vec3 lightPos(0.0f, 0.2f, 0.9f);
	//glm::vec3 lightPos(0.0f, 0.2f, 5.5f);
	glm::mat4 lightProjection, lightView;
	float light_nearPlane = 0.01f, light_farPlane = 5.0f;
	glm::vec3 lightFront(0.0, 0.0, 1.0f);
	//glm::vec3 lightFront(0.0, 0.0, -1.0f); //配5.5f的front
	lightProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, light_nearPlane, light_farPlane);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	setGBuffer();
	setFrameBuffers();
	setAllDepthMap();
	setImages();
	setPbo();

	//定义shader
	const std::string pathstr = "./shaders/";
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader depthPeelingShader((pathstr + "depthPeeling.vs").c_str(), (pathstr + "depthPeeling.fs").c_str());
	Shader lightShader((pathstr + "renderLight.vs").c_str(), (pathstr + "renderLight.fs").c_str());
	Shader calThickSampleShader((pathstr + "calThick.vs").c_str(), (pathstr + "calThick.fs").c_str());
	Shader gBufferShader((pathstr + "gBuffer.vs").c_str(), (pathstr + "gBuffer.fs").c_str());
	Shader sdfThickShader((pathstr + "sdfThick.vs").c_str(), (pathstr + "sdfThick.fs").c_str());
	Shader computeErrorShader((pathstr + "compWithSDF.comp").c_str());

	Shader dualDepthPeelingShader((pathstr + "dualDepthPeeling.vs").c_str(), (pathstr + "dualDepthPeeling.fs").c_str());



	float camera_nearPlane = 0.01f, camera_farPlane = 15.0f;
	quadShader.use();
	quadShader.setInt("uDepthNum", allDepthLayer);
	//绘制透明物体的时候一定要排序，1.绘制所有的不透明物体。2.将透明的物体排序，再从远到近的渲染
	//因为深度测试的原因，如果不按照从远到近的顺序绘制的话，因为深度写入的原因，近的透明的物体
	//的深度会写入到深度缓冲中，然后远处的在这个透明物体的后面的透明物体就会因为深度测试被丢弃
	//因此只能从远到近的绘制
	glm::vec3 staticCameraPos1 = glm::vec3(3.2, 0.2, 2.0);
	glm::vec3 staticCameraFront1 = glm::vec3(-1.0, 0.0, 0.0);
	glm::mat4 staticCameraView1 = glm::lookAt(staticCameraPos1, staticCameraPos1 + staticCameraFront1, glm::vec3(0.0, 1.0, 0.0));
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		initImage();

		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		lightView = glm::lookAt(lightPos, lightPos + lightFront, glm::vec3(0.0, 1.0, 0.0));


		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		gBufferShader.use();
	//	gBufferShader.setMat4("view", staticCameraView1);
		gBufferShader.setMat4("projection", cameraProjection);
		gBufferShader.setMat4("view", cameraView);
		dllRender->renderCubeSet(gBufferShader);
		//dllRender->renderSphereSet(gBufferShader);


		glEnable(GL_BLEND);
		//光源下peeling，用来记录到光源的远近的片段的
		for (int layIdx = 0; layIdx < allDepthLayer; layIdx++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[layIdx%2]);
			//每次循环的时候都需要清除缓冲
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearBufferfv(GL_COLOR, 0, depthClear);
			dualDepthPeelingShader.use();
			dualDepthPeelingShader.setMat4("projection", lightProjection);
			dualDepthPeelingShader.setMat4("view", lightView); 
			dualDepthPeelingShader.setInt("uLayerIndex", layIdx);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frontAndBackDepth[(layIdx+1)%2]);
			//gl_max 没有起作用的原因是没有关闭深度写入，导致只有最前面的片段被保存下来了
			glDepthMask(GL_FALSE);
			dllRender->renderCubeSet(dualDepthPeelingShader);
			//dllRender->renderSphereSet(dualDepthPeelingShader);
			glDepthMask(GL_TRUE);
			glCopyImageSubData(frontAndBackDepth[layIdx % 2], GL_TEXTURE_2D, 0, 0, 0, 0,
				allFrontAndBackDepthMap, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layIdx, SCR_WIDTH, SCR_HEIGHT, 1);
		}

		//从着色点到真实光源下的阴影点形成的线段进行采样并且根据真实光源的OIT进行厚度生成的pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		calThickSampleShader.use();
		calThickSampleShader.setMat4("realLightProjection", lightProjection);
		calThickSampleShader.setMat4("realLightView", lightView);
		calThickSampleShader.setFloat("real_near_plane", light_nearPlane);
		calThickSampleShader.setFloat("real_far_plane", light_farPlane);
		calThickSampleShader.setInt("uDepthNum", allDepthLayer);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, allFrontAndBackDepthMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, frontAndBackDepth[0]); //frontAndBackDepth
		dllRender->renderQuad();

		//dllTextureDisplay->DisplayFramebufferTexture(woodTexture);
		//dllTextureDisplay->DisplayFramebufferTexture(gPositionTex);
		//dllTextureDisplay->DisplayFramebufferTexture(preFrontAndBackDepth);
		//dllTextureDisplay->DisplayFramebufferTexture(frontAndBackDepth[1]);
		//dllTextureDisplay->DisplayFramebufferTextureArray(allFrontAndBackDepthMap,2);
		

		//使用sdf保存thick的pass
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//sdfThickShader.use();
		//sdfThickShader.setMat4("realLightProjection", lightProjection);
		//sdfThickShader.setMat4("realLightView", lightView);
		//sdfThickShader.setFloat("real_near_plane", light_nearPlane);
		//sdfThickShader.setFloat("real_far_plane", light_farPlane);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, gPositionTex);
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, allBackAndFrontDepthMap);
		//dllRender->renderQuad();

		//computeErrorShader.use();
		//glDispatchCompute(40, 23, 1);

		//glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);//可以让等待的区域更小
		//float allError = 0.0f;
		//glBindFramebuffer(GL_FRAMEBUFFER, errorFBO);
		//glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &allError);
		//cout << allError << endl;

		////可视化误差
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		//glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//quadShader.use();
		//quadShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
		//quadShader.setFloat("near_plane", camera_nearPlane);
		//quadShader.setFloat("far_plane", camera_farPlane);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, allBackAndFrontDepthMap);
		//dllRender->renderQuad();


		//在最后添加对光源的渲染是OK的,因为是最后渲染的，所以肯定不会被前面的物体遮挡
		/*lightShader.use();
		lightShader.setMat4("projection", cameraProjection);
		lightShader.setMat4("view", cameraView);
		dllRender->renderSphere(lightShader, lightPos, glm::vec3(lightRadius), lightColor);*/
		//dllTextureDisplay->DisplayFramebufferTexture(interMediateTex);
		//dllTextureDisplay->DisplayFramebufferTexture(woodTexture);
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

void setGBuffer()
{
	//存储相机下看到的最近的片段的世界坐标，即着色点
	glGenTextures(1, &gPositionTex);
	glBindTexture(GL_TEXTURE_2D, gPositionTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &gRboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, gRboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);

	glGenFramebuffers(1, &gBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setFrameBuffers()
{
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, frontAndBackDepth);
	glGenTextures(2, depthMap);

	for (int i = 0; i < 2; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, depthMap[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//光源看不到的地方的深度为1.0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindTexture(GL_TEXTURE_2D, frontAndBackDepth[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//光源看不到的地方的深度为1.0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap[i], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frontAndBackDepth[i], 0);
		glBlendFuncSeparatei(0, GL_ONE, GL_ONE, GL_ONE, GL_ONE);
		glBlendEquationi(0, GL_MAX);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
			std::cout << "Framebuffer::" << i << " not complete!" << std::endl;
		}
	}

}

void setAllDepthMap()
{
	glGenTextures(1, &allFrontAndBackDepthMap);
	glBindTexture(GL_TEXTURE_2D_ARRAY, allFrontAndBackDepthMap);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, allDepthLayer, 0, GL_RG, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

}

void setImages()
{
	glGenTextures(1, &thickNessImage);
	glBindTexture(GL_TEXTURE_2D, thickNessImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(0, thickNessImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	glGenTextures(1, &sdfThickImage);
	glBindTexture(GL_TEXTURE_2D, sdfThickImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(4, sdfThickImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	glGenTextures(1, &errorImage);
	glBindTexture(GL_TEXTURE_2D, errorImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(5, errorImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	glGenFramebuffers(1, &errorFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, errorFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, errorImage, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void setPbo()
{
	glGenBuffers(1, &headPtrInitPBO);
	glBindBuffer(GL_ARRAY_BUFFER, headPtrInitPBO);
	glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_R32UI), NULL, GL_STATIC_DRAW);
	void* initData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memset(initData, 0, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_R32UI));
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initImage()
{
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, headPtrInitPBO);
	glBindTexture(GL_TEXTURE_2D, thickNessImage);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D, sdfThickImage);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D, errorImage);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED, GL_FLOAT, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
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
