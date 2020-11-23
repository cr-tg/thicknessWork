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
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 targetPos(0.0f, 0.0f, 0.0f);//����view�����up�����ĳ�ʼֵ

unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int lightProjVAO;
unsigned int lightProjVBO;

//�ֱ���cube��λ�ã�scale��color��ÿ�ַ���͸��ı���((1-͸����)*��������ɫ)
//float thickness = 0.3f;
//vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>> cubeSceneData
//{
//	make_tuple(glm::vec3(0.0f, 0.2f, 4.0f),glm::vec3(0.4, 0.4, thickness),
//	glm::vec4(0.0, 1.0, 0.0, 0.5),glm::vec3(1.0, 0.0, 1.0)),
//	make_tuple(glm::vec3(0.0f, 0.2f, 3.2f),glm::vec3(0.4, 0.4, thickness),
//	glm::vec4(1.0, 0.0, 0.0, 0.5),glm::vec3(0.0, 1.0, 1.0)),
//};

int main()
{
	//init context,createWindow and make it currentContext
	CWindow::setWindowName("depth_peeling");
	CWindow::setWindowSize(glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
	GLFWwindow* window = CWindow::getOrCreateWindowInstance()->getGlfwWindow();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//����render�������������Ƹ��ּ򵥵���״�����絥��ͼ�Σ�cube��triangle,plane��
	CRender* dllRender = CRender::getOrCreateWindowInstance()->getRender();

	//����displayTexture����������չʾ����texture����Ҫ���������Ե�,������Ϊ��һ���ǲ���texture��Ȼ��show����
	//��˿��Խ�shaderҲ�ں����ڲ���������
	CTextureDisplay* dllTextureDisplay = CTextureDisplay::getOrCreateWindowInstance()->getTextureDisplay();

	unsigned int woodTexture = dllTextureDisplay->loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());

	glEnable(GL_DEPTH_TEST);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };

	//���֡���壬�������ʱ�򣬱���ÿһ�����ɫ����ȣ�Ȼ����ȸ��Ƶ�compareMap�С�����ɫ���Ƶ�colorTexSet��
	//������ɫ����Ϊ֧�ֶ��ز�����,��������Ҫ���ƣ�����˶��ز��������޷�ֱ������ɫ����ʹ�õ�����
	unsigned int singleMsaaColorFBO;
	glGenFramebuffers(1, &singleMsaaColorFBO);

	int offscreenSample = 4;
	unsigned int singleMsaaColorTex;
	glGenTextures(1, &singleMsaaColorTex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, singleMsaaColorTex);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, offscreenSample, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	//���ز�����������ָ��GL_TEXTURE_MIN_FILTER/MAGFILTER����Ϊ���ز��������ܹ���
	//Ҳ����ָ����������Ĳ���

	unsigned int singleMsaaDepthMap;
	glGenTextures(1, &singleMsaaDepthMap);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, singleMsaaDepthMap);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, offscreenSample, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, singleMsaaColorFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, singleMsaaColorTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, singleMsaaDepthMap, 0);
	//����������仰�����Ҹ��˰��죬��Ϊ��Ҫ��buffer����read ��color����ɫ��Ȼ��draw����ͨ�������ϣ�������Ҫ��read�ĸ���
	//glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
		std::cout << "Framebuffer::depthMapFBO not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//���֡���壬�����������в����ɫ��һ����ȵ�compareMap
	unsigned int allColorFBO;
	glGenFramebuffers(1, &allColorFBO);

	unsigned int allColorTex;
	glGenTextures(1, &allColorTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, allColorTex);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, allDepthLayer, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	unsigned int preDepthMap;
	glGenTextures(1, &preDepthMap);
	glBindTexture(GL_TEXTURE_2D, preDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//��Դ�������ĵط������Ϊ1.0
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, allColorFBO);
	for (int i = 0; i < allDepthLayer; ++i)
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, allColorTex, 0, i);
	}
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, preDepthMap, 0);
	vector<GLuint> attachments;
	for (int i = 0; i < allDepthLayer; ++i)
	{
		attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers(allDepthLayer, attachments.data());
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
		std::cout << "Framebuffer::colorMapFBO not complete!" << std::endl;
	}
	//��Ϊ�޷�ֱ�ӽ����ز��������Ƶ���ͨ2D�����ĳһ���У�������Ҫ�Ƚ�������msaa�������֡������
	unsigned int interMediateFBO;
	glGenFramebuffers(1, &interMediateFBO);
	unsigned int interMediateTex;
	glGenTextures(1, &interMediateTex);
	glBindTexture(GL_TEXTURE_2D, interMediateTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	unsigned int interDepthMap;
	glGenTextures(1, &interDepthMap);
	glBindTexture(GL_TEXTURE_2D, interDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, interMediateTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, interDepthMap, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "interMediateFBO not complete��" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//����shader
	const std::string pathstr = "./shaders/";
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader depthPeelingShader((pathstr + "depthPeeling.vs").c_str(), (pathstr + "depthPeeling.fs").c_str());
	Shader blendShader((pathstr + "blend.vs").c_str(), (pathstr + "blend.fs").c_str());

	float camera_nearPlane = 0.1f, camera_farPlane = 10.0f;
	quadShader.use();
	quadShader.setInt("uDepthNum", allDepthLayer);
	//����͸�������ʱ��һ��Ҫ����1.�������еĲ�͸�����塣2.��͸�������������ٴ�Զ��������Ⱦ
	//��Ϊ��Ȳ��Ե�ԭ����������մ�Զ������˳����ƵĻ�����Ϊ���д���ԭ�򣬽���͸��������
	//����Ȼ�д�뵽��Ȼ����У�Ȼ��Զ���������͸������ĺ����͸������ͻ���Ϊ��Ȳ��Ա�����
	//���ֻ�ܴ�Զ�����Ļ���
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		glClearColor(0.10f, 0.10f, 0.10f, 0.0f);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		depthPeelingShader.use();
		depthPeelingShader.setMat4("projection", cameraProjection);
		depthPeelingShader.setMat4("view", cameraView);
		for (int layIdx = 0; layIdx < allDepthLayer; layIdx++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
			//ÿ��ѭ����ʱ����Ҫ�������
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			depthPeelingShader.use();
			depthPeelingShader.setInt("uLayerIndex", layIdx);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, preDepthMap);//allColorFBO�е����������
			depthPeelingShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
			dllRender->renderPlane(depthPeelingShader);
			//�������ṩ�ļ������Ϊpcf����,��������Ⱦѭ������
			dllRender->renderSingFace(depthPeelingShader, glm::vec3(0.0f, 0.2f, 4.0f), glm::vec4(0.0, 1.0, 0.0, 0.5));
			dllRender->renderSingFace(depthPeelingShader, glm::vec3(0.0f, 0.2f, 3.5f), glm::vec4(1.0, 0.0, 0.0, 0.4));
			dllRender->renderSingFace(depthPeelingShader, glm::vec3(0.0f, 0.2f, 3.0f), glm::vec4(0.0, 0.0, 1.0, 0.3));
			dllRender->renderSingFace(depthPeelingShader, glm::vec3(0.0f, 0.2f, 2.5f), glm::vec4(1.0, 1.0, 0.0, 0.2));
			dllRender->renderSingFace(depthPeelingShader, glm::vec3(0.0f, 0.2f, 2.0f), glm::vec4(1.0, 0.0, 1.0, 0.1));
			dllRender->renderSingFace(depthPeelingShader, glm::vec3(0.0f, 0.2f, 1.5f), glm::vec4(0.0, 1.0, 1.0, 0.6));
			
			//dllRender->renderSceneFromData(depthPeelingShader, cubeData);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, interMediateFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, allColorFBO);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT,
				GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			//��������msaa��������������Ϊ��Ա�Ч�ʣ���ȥ����msaa��һ��
		/*	glBindFramebuffer(GL_READ_FRAMEBUFFER, singleMsaaColorFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, allColorFBO);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT,
				GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, singleMsaaColorFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, interMediateFBO);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);*/
			//�������������sample����Ҫmatch����,������Ȱ�msaa����ɫ���Ƶ�interMediate�У�
			//�ٴ�interMediate���Ƶ�allColorTex��
			glCopyImageSubData(interMediateTex, GL_TEXTURE_2D, 0, 0, 0, 0,
				allColorTex, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layIdx, SCR_WIDTH, SCR_HEIGHT, 1);
		}

		//blend
		blendShader.use();
		blendShader.setInt("uAllDepthLayer", allDepthLayer);
		for (int layIdx = allDepthLayer -1; layIdx > -1; layIdx--)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
			//ÿ��ѭ����ʱ����Ҫ�������
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, allColorTex);
			blendShader.setInt("uLayerIndex", layIdx);
			dllRender->renderQuad();
		/*	glBindFramebuffer(GL_READ_FRAMEBUFFER, singleMsaaColorFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, interMediateFBO);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);*/
			//�������������sample����Ҫmatch����
			glCopyImageSubData(interMediateTex, GL_TEXTURE_2D, 0, 0, 0, 0,
				allColorTex, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layIdx, SCR_WIDTH, SCR_HEIGHT, 1);
		}
		//���ӻ�
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quadShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, allColorTex);
		dllRender->renderQuad();
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
	glDeleteFramebuffers(1, &singleMsaaColorFBO);

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
