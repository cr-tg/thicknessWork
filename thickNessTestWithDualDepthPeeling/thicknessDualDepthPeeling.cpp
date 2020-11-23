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

//������ɫ����ȫ�ֹ������С
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

//��������
unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int lightProjVAO;
unsigned int lightProjVBO;

GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat depthClear[] = { -1.0, -1.0, -1.0, -1.0 };

//gbuffer�е�����
unsigned int gPositionTex;
unsigned int gRboDepth;
unsigned int gBufferFBO;
void setGBuffer();

//ÿһpass��������һ�θ���ȷʵ��ɵ����˻���ʹ��pingpongbuffer�Ľ������

//ÿ�ΰ��룬����һ�����Ⱥ���ɫ���浽��һ�εĽ���У�Ȼ���ٽ���ȸ��Ƶ�allDepthMap�Ķ�Ӧ���У���ɫ���Ƶ�allColorFBO��

unsigned int pingpongFBO[2];
unsigned int frontAndBackDepth[2];
unsigned int depthMap[2];
void setFrameBuffers();

unsigned int allFrontAndBackDepthMap;//�������в��ǰ���������ȵ���������
void setAllDepthMap();

//����thick�ı������ͼ���ƣ����ͼ�洢������������ȣ�����洢��������㵽�����ĵ�ĺ��
unsigned int thickNessImage;
unsigned int sdfThickImage;//�洢��sdf����ȣ��������Ƚϵ�
unsigned int errorImage;//�洢����
unsigned int errorFBO;//��Ϊ���image��ͨ��compute shader������ģ������Ҫһ��fbo����Ϊ���
void setImages();

GLuint headPtrInitPBO;//ʹ��PBO��ʼ��thick��,pbo�е�ֵȫ��Ϊ0
void setPbo();

void initImage();//ÿһ֡��ʼ�׶��ȳ�ʼ������image


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

	//����render�������������Ƹ��ּ򵥵���״�����絥��ͼ�Σ�cube��triangle,plane��
	CRender* dllRender = CRender::getOrCreateWindowInstance()->getRender();
	CTextureDisplay* dllTextureDisplay = CTextureDisplay::getOrCreateWindowInstance()->getTextureDisplay();
	unsigned int woodTexture = dllTextureDisplay->loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());
	
	glm::vec3 lightPos(0.0f, 0.2f, 0.9f);
	//glm::vec3 lightPos(0.0f, 0.2f, 5.5f);
	glm::mat4 lightProjection, lightView;
	float light_nearPlane = 0.01f, light_farPlane = 5.0f;
	glm::vec3 lightFront(0.0, 0.0, 1.0f);
	//glm::vec3 lightFront(0.0, 0.0, -1.0f); //��5.5f��front
	lightProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, light_nearPlane, light_farPlane);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	setGBuffer();
	setFrameBuffers();
	setAllDepthMap();
	setImages();
	setPbo();

	//����shader
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
	//����͸�������ʱ��һ��Ҫ����1.�������еĲ�͸�����塣2.��͸�������������ٴ�Զ��������Ⱦ
	//��Ϊ��Ȳ��Ե�ԭ����������մ�Զ������˳����ƵĻ�����Ϊ���д���ԭ�򣬽���͸��������
	//����Ȼ�д�뵽��Ȼ����У�Ȼ��Զ���������͸������ĺ����͸������ͻ���Ϊ��Ȳ��Ա�����
	//���ֻ�ܴ�Զ�����Ļ���
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
		//��Դ��peeling��������¼����Դ��Զ����Ƭ�ε�
		for (int layIdx = 0; layIdx < allDepthLayer; layIdx++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[layIdx%2]);
			//ÿ��ѭ����ʱ����Ҫ�������
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearBufferfv(GL_COLOR, 0, depthClear);
			dualDepthPeelingShader.use();
			dualDepthPeelingShader.setMat4("projection", lightProjection);
			dualDepthPeelingShader.setMat4("view", lightView); 
			dualDepthPeelingShader.setInt("uLayerIndex", layIdx);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frontAndBackDepth[(layIdx+1)%2]);
			//gl_max û�������õ�ԭ����û�йر����д�룬����ֻ����ǰ���Ƭ�α�����������
			glDepthMask(GL_FALSE);
			dllRender->renderCubeSet(dualDepthPeelingShader);
			//dllRender->renderSphereSet(dualDepthPeelingShader);
			glDepthMask(GL_TRUE);
			glCopyImageSubData(frontAndBackDepth[layIdx % 2], GL_TEXTURE_2D, 0, 0, 0, 0,
				allFrontAndBackDepthMap, GL_TEXTURE_2D_ARRAY, 0, 0, 0, layIdx, SCR_WIDTH, SCR_HEIGHT, 1);
		}

		//����ɫ�㵽��ʵ��Դ�µ���Ӱ���γɵ��߶ν��в������Ҹ�����ʵ��Դ��OIT���к�����ɵ�pass
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
		

		//ʹ��sdf����thick��pass
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

		//glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);//�����õȴ��������С
		//float allError = 0.0f;
		//glBindFramebuffer(GL_FRAMEBUFFER, errorFBO);
		//glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &allError);
		//cout << allError << endl;

		////���ӻ����
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


		//�������ӶԹ�Դ����Ⱦ��OK��,��Ϊ�������Ⱦ�ģ����Կ϶����ᱻǰ��������ڵ�
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
	//�洢����¿����������Ƭ�ε��������꣬����ɫ��
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//��Դ�������ĵط������Ϊ1.0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindTexture(GL_TEXTURE_2D, frontAndBackDepth[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//��Դ�������ĵط������Ϊ1.0
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
