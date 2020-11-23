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
#define lightRadius 0.04f
// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 736;

//������ɫ����ȫ�ֹ������С
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
	CWindow::setWindowName("OIT_THICK");
	CWindow::setWindowSize(glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
	GLFWwindow* window = CWindow::getOrCreateWindowInstance()->getGlfwWindow();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowPos(window, 100, 100);

	//����render�������������Ƹ��ּ򵥵���״�����絥��ͼ�Σ�cube��triangle,plane��
	CRender* dllRender = CRender::getOrCreateWindowInstance()->getRender();

	//����displayTexture����������չʾ����texture����Ҫ���������Ե�,������Ϊ��һ���ǲ���texture��Ȼ��show����
	//��˿��Խ�shaderҲ�ں����ڲ���������
	CTextureDisplay* dllTextureDisplay = CTextureDisplay::getOrCreateWindowInstance()->getTextureDisplay();

	
	unsigned int woodTexture = dllTextureDisplay->loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());
	glEnable(GL_DEPTH_TEST);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };

	

	//����shader
	const std::string pathstr = "./shaders/";
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader linkListCamShader((pathstr + "link_list_cam.vs").c_str(), (pathstr + "link_list_cam.fs").c_str());
	//�����Ǵ���cam��һ��shader��light��һ��shader�ģ����Ƿ��������Ļ�����Ҫ����ԭ�Ӽ�������ͷ�����о��˷���Ч��
	//��˽�node��w����������Դ�µ���ȣ��Լ�������ı任,��ʵ�Լ����任Ҳͦ�򵥣��ͺ�shadow mapһ��
	//������ʹ��һ�������pass����Ϊcam�ƶ��Ժ󣬿�����ԭ����lightCoordλ���£����Ѿ�û��Ƭ���ˣ���ʱ����������Ƭ����ɫ��
	Shader linkListLightShader((pathstr + "link_list_light.vs").c_str(), (pathstr + "link_list_light.fs").c_str());
	Shader linkListSampleSortShader((pathstr + "sortAndBlend.vs").c_str(), (pathstr + "sortAndBlend.fs").c_str());
	Shader lightShader((pathstr + "renderLight.vs").c_str(), (pathstr + "renderLight.fs").c_str());
	Shader gBufferShader((pathstr + "gBuffer.vs").c_str(), (pathstr + "gBuffer.fs").c_str());
	Shader simpleDepthShader((pathstr + "simpleShadow.vs").c_str(), (pathstr + "simpleShadow.fs").c_str());
	Shader calOITThickShader((pathstr + "calThick.vs").c_str(), (pathstr + "calThick.fs").c_str());
	Shader sdfThickShader((pathstr + "sdfThick.vs").c_str(), (pathstr + "sdfThick.fs").c_str());
	Shader computeErrorShader((pathstr + "compWithSDF.comp").c_str());

	linkListCamShader.use();
	GLuint64 maxListNodeNum = SCR_WIDTH * SCR_HEIGHT * listLayer;
	GLuint64 nodeSize = sizeof(glm::uvec4);
	GLuint64 ssboCamNodeSize = sizeof(glm::uvec3);
	GLuint64 ssboLightNodeSize = sizeof(glm::uvec2);
	linkListCamShader.setInt("uMaxListNode", maxListNodeNum);
	
	//glm::vec3 lightPos(0.0f, 0.2f, -0.1f);
	glm::vec3 lightPos(0.0f, 0.2f, 0.9f);
	glm::vec3 lightFront(0.0, 0.0, 1.0f);
	glm::mat4 lightProjection, lightView;
	float light_nearPlane = 0.01f, light_farPlane = 5.0f;
	glm::vec3 targetPos(0.0f, 0.0f, 0.0f);
	lightProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, light_nearPlane, light_farPlane);
	linkListLightShader.use();
	linkListLightShader.setInt("uMaxListNode", maxListNodeNum);
	linkListLightShader.setMat4("projection", lightProjection);

	//�洢����¿����������Ƭ�ε��������꣬����ɫ��
	unsigned int gPositionTex;
	glGenTextures(1, &gPositionTex);
	glBindTexture(GL_TEXTURE_2D, gPositionTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned int gNormalTex;
	glGenTextures(1, &gNormalTex);
	glBindTexture(GL_TEXTURE_2D, gNormalTex);
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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalTex, 0);
	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//��ʵ��Դ�µ���������
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



	unsigned int blendingColorFBO;
	glGenFramebuffers(1, &blendingColorFBO);

	unsigned int blendingColorTex;
	glGenTextures(1, &blendingColorTex);
	glBindTexture(GL_TEXTURE_2D, blendingColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	unsigned int blendingDepthMap;
	glGenTextures(1, &blendingDepthMap);
	glBindTexture(GL_TEXTURE_2D, blendingDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, blendingColorFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blendingColorTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, blendingDepthMap, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//������������ԭ�Ӽ����������ԭ�Ӽ���������ÿ��Ƭ��Ψһ����ű�ʶ
	unsigned int atomicBuffer;
	glGenBuffers(1, &atomicBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint64), nullptr, GL_DYNAMIC_COPY);
	const int one = 1;
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint64), &one);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	unsigned int headPtrImage;//ͷ�ڵ��
	glGenTextures(1, &headPtrImage);
	glBindTexture(GL_TEXTURE_2D, headPtrImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);//�����levelֻ����1
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(1, headPtrImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);//��Ϊimage

	unsigned int lightHeadPtrImage;//������ڹ�Դ�ӽ��µ�ͷ����
	glGenTextures(1, &lightHeadPtrImage);
	glBindTexture(GL_TEXTURE_2D, lightHeadPtrImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);//�����levelֻ����1
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(2, lightHeadPtrImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);//��Ϊimage

	//ʹ��PBO��ʼ��ͷ����
	GLuint headPtrInitPBO;
	glGenBuffers(1, &headPtrInitPBO);
	glBindBuffer(GL_ARRAY_BUFFER, headPtrInitPBO);
	glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_R32UI), NULL, GL_STATIC_DRAW);
	void* initData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memset(initData, 0, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_R32UI));
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	unsigned int thickNessImage;//����thick�ı������ͼ���ƣ����ͼ�洢������������ȣ�����洢��������㵽�����ĵ�ĺ��
	glGenTextures(1, &thickNessImage);
	glBindTexture(GL_TEXTURE_2D, thickNessImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(3, thickNessImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	unsigned int sdfThickImage;//�洢��sdf����ȣ��������Ƚϵ�
	glGenTextures(1, &sdfThickImage);
	glBindTexture(GL_TEXTURE_2D, sdfThickImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(4, sdfThickImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	unsigned int errorImage;//�洢����
	glGenTextures(1, &errorImage);
	glBindTexture(GL_TEXTURE_2D, errorImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(5, errorImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	unsigned int imageReadFBO;
	glGenFramebuffers(1, &imageReadFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, imageReadFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, errorImage, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int ssboCam;//�洢����Ƭ�ε�ssboCam���������ӵ���ɫ����0��λ��
	glGenBuffers(1, &ssboCam);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCam);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxListNodeNum* ssboCamNodeSize, NULL, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboCam);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	unsigned int ssboLight;
	glGenBuffers(1, &ssboLight);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboLight);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxListNodeNum* ssboLightNodeSize, NULL, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboLight);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	float camera_nearPlane = 0.01f, camera_farPlane = 15.0f;
	quadShader.use();
	quadShader.setFloat("far_plane", light_farPlane);
	quadShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
	linkListSampleSortShader.use();
	linkListSampleSortShader.setFloat("far_plane", light_farPlane);
	linkListSampleSortShader.setFloat("near_plane", light_nearPlane);
	linkListSampleSortShader.setVec2("uScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));

	glm::vec3 staticCameraPos1 = glm::vec3(3.2, 0.2, 2.0);
	glm::vec3 staticCameraFront1 = glm::vec3(-1.0, 0.0, 0.0);
	glm::mat4 staticCameraView1 = glm::lookAt(staticCameraPos1, staticCameraPos1 + staticCameraFront1, glm::vec3(0.0, 1.0, 0.0));

	glm::vec3 staticCameraPos2 = glm::vec3(0.68, 0.25, 4.0);
	glm::vec3 staticCameraFront2 = glm::vec3(0.3, -0.22, -0.93);
	glm::mat4 staticCameraView2 = glm::lookAt(staticCameraPos2, staticCameraPos2 + staticCameraFront2, glm::vec3(0.0, 1.0, 0.0));
	while (!glfwWindowShouldClose(window))
	{
		//cout << "x:" << camera.Position.x << ",y:" << camera.Position.y << ",z:" << camera.Position.z << endl;
		//cout << "fornt x:" << camera.Front.x << ",y:" << camera.Front.y << ",z:" << camera.Front.z << endl;
		//��ʼ��ԭ�Ӽ�����Ϊ1
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint64), &one);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		//��ʼ��ͷ�ڵ�image������Ϊ0
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, headPtrInitPBO);
		glBindTexture(GL_TEXTURE_2D, headPtrImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, lightHeadPtrImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, thickNessImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, sdfThickImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, errorImage);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED, GL_FLOAT, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);


		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		static glm::mat4 staCameraView = camera.GetViewMatrix();
		static glm::mat4 staCameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		lightView = glm::lookAt(lightPos, lightPos + lightFront, glm::vec3(0.0, 1.0, 0.0));

		glClearColor(0.10f, 0.10f, 0.10f, 0.0f);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		//��ʵ��Դ�µ�shadow map
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		simpleDepthShader.use();
		simpleDepthShader.setMat4("projection", lightProjection);
		simpleDepthShader.setMat4("view", lightView);
		//dllRender->renderCubeSetFromData(simpleDepthShader, cubeSceneData);
		//dllRender->renderSphereSet(simpleDepthShader);
		dllRender->testErrorRateScene(simpleDepthShader);

		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gBufferShader.use();
		gBufferShader.setMat4("projection", cameraProjection);
		gBufferShader.setMat4("view", cameraView);
		//gBufferShader.setMat4("view", staticCameraView2);
		//	gBufferShader.setMat4("view", staticCameraView1);
		//dllRender->renderCubeSetFromData(gBufferShader, cubeSceneData);
		//dllRender->renderSphereSet(gBufferShader);
		dllRender->testErrorRateScene(gBufferShader);

		glBindFramebuffer(GL_FRAMEBUFFER, blendingColorFBO);
		//��ʵ��Դ������OIT��pass ����¼��ssboLight��
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		linkListLightShader.use();
		linkListLightShader.setMat4("view", lightView);
		glDepthMask(GL_FALSE);
		//dllRender->renderCubeSetFromData(linkListLightShader, cubeSceneData);
		//dllRender->renderSphereSet(linkListLightShader);
		dllRender->testErrorRateScene(linkListLightShader);
		glDepthMask(GL_TRUE);

		//��ʼ��ԭ�Ӽ�����Ϊ1
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint64), &one);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		//camera��OIT��pass����¼��ssboCam�У����pass��Ҫ������͸����ɫ��
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		linkListCamShader.use(); 
		linkListCamShader.setMat4("projection", cameraProjection);
		linkListCamShader.setMat4("view", cameraView);
		//linkListShader.setMat4("lightProjection", cameraProjection);
		//linkListShader.setMat4("lightView", cameraView);
		//��͸�������壬д�뵽��Ȼ�����
		linkListCamShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
	//	dllRender->renderPlane(linkListCamShader);
		glDepthMask(GL_FALSE);
		//dllRender->renderCubeSetFromData(linkListCamShader, cubeSceneData);
		//dllRender->renderSphereSet(linkListCamShader);
		dllRender->testErrorRateScene(linkListCamShader);
		glDepthMask(GL_TRUE);
		//glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);//�����õȴ��������С
		//����pass
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		linkListSampleSortShader.use();
		dllRender->renderQuad();


		//����ɫ�㵽��ʵ��Դ�µ���Ӱ���γɵ��߶ν��в������Ҹ�����ʵ��Դ��OIT���к�����ɵ�pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		calOITThickShader.use();
		calOITThickShader.setMat4("realLightProjection", lightProjection);
		calOITThickShader.setMat4("realLightView", lightView);
		calOITThickShader.setFloat("real_near_plane", light_nearPlane);
		calOITThickShader.setFloat("real_far_plane", light_farPlane);
		calOITThickShader.setVec3("uLightPos", lightPos);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, realLightDepthMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gNormalTex);
		dllRender->renderQuad();
		//glDisable(GL_DEPTH_TEST);//��������Ȳ����Ժ󣬲��ܹ���Ⱦ��light
		//lightShader.use();
		//lightShader.setMat4("projection", cameraProjection);
		////lightShader.setMat4("view", cameraView);
		//lightShader.setMat4("view", staticCameraView2);
		//dllRender->renderSphere(lightShader, glm::vec3(0.9, 0.2, 4.0), glm::vec3(0.05f), lightColor);
		//glEnable(GL_DEPTH_TEST);


		//ʹ��sdf����thick��pass
	/*	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

		computeErrorShader.use();
		glDispatchCompute(40, 23, 1);

		glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);//�����õȴ��������С
		float allError = 0.0f;
		glBindFramebuffer(GL_FRAMEBUFFER, imageReadFBO);
		glReadPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &allError);
		cout << allError << endl;*/

		
		//���ӻ�
		/*glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quadShader.use();
		glActiveTexture(GL_TEXTURE0); 
		glBindTexture(GL_TEXTURE_2D, blendingColorTex);
		//glBindTexture(GL_TEXTURE_2D, gPositionTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, headPtrImage);
		dllRender->renderQuad();
		//�������ӶԹ�Դ����Ⱦ��OK��,��Ϊ�������Ⱦ�ģ����Կ϶����ᱻǰ��������ڵ�
		lightShader.use();
		lightShader.setMat4("projection", cameraProjection);
		lightShader.setMat4("view", cameraView);
		//lightShader.setMat4("view", staticCameraView1);
		dllRender->renderSphere(lightShader, lightPos, glm::vec3(lightRadius), lightColor);
		glEnable(GL_DEPTH_TEST);*/
	
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
