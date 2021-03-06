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
void renderQuad();
void renderCube();
void renderLight(const Shader& shader, glm::vec3 lightPos);
void renderScene(const Shader& shader);
void renderSceneToDepth(const Shader& shader);
void renderPlane(const Shader& shader);
unsigned int loadTexture(char const* path);
void DisplayFramebufferTexture(GLuint textureID);//调试用的函数，显示纹理屏幕上
void DisplayFramebufferTextureArray(GLuint textureID);
void DisplayFramebufferTextureMsaa(GLuint textureID);
void renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec3& faceColor);

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
glm::vec3 targetPos(0.0f, 0.0f, 0.0f);//测试view矩阵的up向量的初始值

unsigned int planeVAO;
unsigned int planeVBO;
unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int singleFaceVAO;
unsigned int singleFaceVBO;

//分别是cube的位置，scale，color，每种分量透射的比例((1-透明度)*分量的颜色)
float thickness = 0.3f;
vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>> cubeSceneData
{

	//场景2：一个大方块包裹住一个小方块
	//此时因为两个物体的坐标是一样的，因此它们到camera的距离也是一样的，
	//此时就无法在cpu端排序了，如果使用之前的map方法，两个key一样的，只会绘制出一个物体出来
	//如果先绘制大的，再绘制小的，在矩形物体外面只看得见大的物体看不见小的物体
	//因为大的物体写入深度以后，深度测试将小的物体给排除出去了，只有进入物体内部
	//才能够看见小的物体
	//如果先绘制小的，再绘制大的，那么就是OK的
	//如果禁用掉深度，那当然是不会有遮挡的存在，但是这样的话，做混合的时候，会搞不清谁
	//是src，谁是dst，会造成混合的错误
	/*make_tuple(glm::vec3(1.5f, 0.0f, 1.5),glm::vec3(1.0, 1.0, 1.0),
	glm::vec4(0.0, 1.0, 1.0, 0.4),glm::vec3(0.0, 1.0, 1.0)),
		make_tuple(glm::vec3(1.5f, 0.0f, 1.5),glm::vec3(0.5, 0.5, 0.5),
	glm::vec4(0.0, 0.0, 1.0, 0.4),glm::vec3(0.0, 0.0, 1.0)),*/
	/*make_tuple(glm::vec3(0.0f, 0.2f, 3.5f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(0.0, 1.0, 0.0, 0.4),glm::vec3(1.0, 0.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 2.5f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(0.0, 1.0, 1.0, 0.4),glm::vec3(1.0, 0.0, 0.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 1.5f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(1.0, 0.0, 0.0, 0.4),glm::vec3(0.0, 1.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 0.5f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(1.0, 1.0, 0.0, 0.4),glm::vec3(0.0, 0.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, -0.5f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(0.0, 0.0, 1.0, 0.4),glm::vec3(1.0, 1.0, 0.0)),*/

	//make_tuple(glm::vec3(0.0f, 0.2f, 3.2f),glm::vec3(0.4, 0.4, thickness),
	//glm::vec4(1.0, 0.0, 0.0, 0.5),glm::vec3(0.0, 1.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 4.0f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(0.0, 1.0, 0.0, 0.5),glm::vec3(1.0, 0.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 3.2f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(1.0, 0.0, 0.0, 0.5),glm::vec3(0.0, 1.0, 1.0)),

};
//CSSM的pcf采样的偏移
vector<glm::vec2> offsetVec
{
	glm::vec2(0,0),
	glm::vec2(3,3),glm::vec2(-3,3),glm::vec2(3,-3),glm::vec2(-3,-3),
	glm::vec2(4,0),glm::vec2(-4,0),glm::vec2(0,4),glm::vec2(0,-4),
	glm::vec2(7,0),glm::vec2(-7,0),glm::vec2(0,7),glm::vec2(0,-7)
};

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif
	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetWindowPos(window, 200, 200);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str());
	//USAGE的区别：见buffer总结

	//定义一个matrix buffer，用来存储camera的view 和projection
	unsigned int vpMatrixBuffer;
	glGenBuffers(1, &vpMatrixBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, vpMatrixBuffer);
	//从主程序获取的数据就使用draw，从shader中获取的数据就使用copy
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	//用来保存cssm深度图的帧缓冲，因为需要三个分量，因此使用大小为3的纹理数组来保存
	unsigned int cssmDepthFBO;
	glGenFramebuffers(1, &cssmDepthFBO);
	unsigned int depthMapArray;
	glGenTextures(1, &depthMapArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depthMapArray);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 3, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//如果想要使用sampler2DArrayShadow 必须启用纹理比较
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, cssmDepthFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapArray, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//这个帧缓冲，在排序的时候，保存每一层的颜色和深度，然后将深度复制到compareMap中。将颜色复制到colorTexSet中
	//并且颜色纹理为支持多重采样的,正好它需要复制，解决了多重采样纹理无法直接在着色器中使用的问题
	unsigned int msaaColorFBO;
	glGenFramebuffers(1, &msaaColorFBO);

	unsigned int firstDepthMap;
	glGenTextures(1, &firstDepthMap);
	glBindTexture(GL_TEXTURE_2D, firstDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int firstDepthFBO;
	glGenFramebuffers(1, &firstDepthFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, firstDepthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, firstDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	int offscreenSample = 4;
	unsigned int singleMsaaColorTex;
	glGenTextures(1, &singleMsaaColorTex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, singleMsaaColorTex);
	//因为是对blend以后的结果做a2c的操作，因此需要hdr纹理，否则比如你的alpha值为0.5，那么最后显示的结果最大就为127
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, offscreenSample, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);

	unsigned int singleMsaaDepthMap;
	glGenTextures(1, &singleMsaaDepthMap);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, singleMsaaDepthMap);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, offscreenSample, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, msaaColorFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, singleMsaaColorTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, singleMsaaDepthMap, 0);
	//就是下面这句话，让我改了半天，因为需要从buffer里面read 出color的颜色，然后draw到普通的纹理上，所以需要有read的附件
	//glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
		std::cout << "Framebuffer::depthMapFBO not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//这个帧缓冲，用来保存所有层的颜色和一个深度的compareMap
	//因为无法直接将多重采样纹理复制到普通2D纹理的某一层中，所以需要先将离屏的msaa传到这个帧缓冲中
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
	unsigned int interRBO;
	glGenRenderbuffers(1, &interRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, interRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, interMediateTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, interRBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "interMediateFBO not complete！" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//定义shader
	const std::string pathstr = "./shaders/";
	Shader cssmDepthShader((pathstr + "shadow_depth.vs").c_str(), (pathstr + "shadow_depth.fs").c_str(),
		(pathstr + "shadow_depth.geom").c_str());
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad_depth.fs").c_str());
	Shader texShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "texShow.fs").c_str());
	Shader texArrayShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "texArrayShow.fs").c_str());
	Shader texMsaaShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "msaaTexShow.fs").c_str());
	Shader alpha2cRenderShader((pathstr + "alpha2cRender.vs").c_str(), (pathstr + "alpha2cRender.fs").c_str());
	Shader alpha2cResolveShader((pathstr + "alpha2cResolve.vs").c_str(), (pathstr + "alpha2cResolve.fs").c_str());
	Shader sc2DoorShader((pathstr + "screen2Door.vs").c_str(), (pathstr + "screen2Door.fs").c_str());


	glm::vec3 lightPos(-2.0f, 1.0f, -1.0f);
	glm::mat4 lightProjection, lightView;
	float near_plane = 2.0f, far_plane = 10.5f;
	float camera_nearPlane = 0.1f, camera_farPlane = 10.0f;
	lightProjection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, near_plane, far_plane);
	//不应该使用透视投影，因为这是用在点光源阴影的生成中的，需要万向投影贴图
	//lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane);
	lightView = glm::lookAt(lightPos, targetPos, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 inverseInitLightView = glm::inverse(lightView);
	texShowShader.use();
	texShowShader.setInt("uTexture", 0);
	texArrayShowShader.use();
	texArrayShowShader.setInt("uTextureArray", 0);
	cssmDepthShader.use();
	cssmDepthShader.setMat4("projection", lightProjection);
	cssmDepthShader.setMat4("view", lightView);
	quadShader.use();
	quadShader.setFloat("near_plane", near_plane);
	quadShader.setFloat("far_plane", far_plane);
	quadShader.setInt("colorTexSet", 0);
	quadShader.setInt("colorTex", 1);
	quadShader.setInt("shadowMap", 2);
	//绘制透明物体的时候一定要排序，1.绘制所有的不透明物体。2.将透明的物体排序，再从远到近的渲染
	//因为深度测试的原因，如果不按照从远到近的顺序绘制的话，因为深度写入的原因，近的透明的物体
	//的深度会写入到深度缓冲中，然后远处的在这个透明物体的后面的透明物体就会因为深度测试被丢弃
	//因此只能从远到近的绘制
	int lastPcfCount = -1;
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		glBindBuffer(GL_UNIFORM_BUFFER, vpMatrixBuffer);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, vpMatrixBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(cameraView), &cameraView);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(cameraView), sizeof(cameraProjection), &cameraProjection);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, cssmDepthFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		cssmDepthShader.use();
		renderPlane(cssmDepthShader);
		renderSceneToDepth(cssmDepthShader);



		/*alpha2cRenderShader.use();
		glBindFramebuffer(GL_FRAMEBUFFER, firstDepthFBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderPlane(alpha2cRenderShader);
		renderScene(alpha2cRenderShader);

		glBindFramebuffer(GL_FRAMEBUFFER, msaaColorFBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		alpha2cRenderShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
		renderPlane(alpha2cRenderShader);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
		glDepthMask(GL_FALSE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, firstDepthMap);
		renderScene(alpha2cRenderShader);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);*/

		sc2DoorShader.use();
		glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sc2DoorShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
		//renderPlane(sc2DoorShader);
		//renderScene(sc2DoorShader);
		renderSingFace(sc2DoorShader, glm::vec3(0.0f, 0.2f, 4.0f), glm::vec3(0.0, 1.0, 0.0));
		renderSingFace(sc2DoorShader, glm::vec3(0.0f, 0.2f, 3.5f), glm::vec3(1.0, 0.0, 0.0));
		renderSingFace(sc2DoorShader, glm::vec3(0.0f, 0.2f, 3.0f), glm::vec3(0.0, 0.0, 1.0));
		renderSingFace(sc2DoorShader, glm::vec3(0.0f, 0.2f, 2.5f), glm::vec3(1.0, 1.0, 0.0));
		renderSingFace(sc2DoorShader, glm::vec3(0.0f, 0.2f, 2.0f), glm::vec3(1.0, 0.0, 1.0));
		renderSingFace(sc2DoorShader, glm::vec3(0.0f, 0.2f, 1.5f), glm::vec3(0.0, 1.0, 1.0));

		//glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaColorFBO);
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, interMediateFBO);
		//glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT,
			//GL_COLOR_BUFFER_BIT, GL_LINEAR);

	/*	alpha2cResolveShader.use();
		alpha2cResolveShader.setInt("uSampleNum", offscreenSample);
		glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, singleMsaaColorTex);
		renderQuad();
		glEnable(GL_DEPTH_TEST);*/

		//可视化
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quadShader.use();
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, interMediateTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, firstDepthMap);
		renderQuad();
		glEnable(GL_DEPTH_TEST);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &quadVBO);
	glDeleteFramebuffers(1, &cssmDepthFBO);
	glDeleteFramebuffers(1, &msaaColorFBO);

	glfwTerminate();
	return 0;
}

void renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec3& faceColor)
{
	if (singleFaceVAO == 0)
	{
		float planeVertices[] = {
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
		};
		glGenVertexArrays(1, &singleFaceVAO);
		glGenBuffers(1, &singleFaceVBO);
		glBindVertexArray(singleFaceVAO);
		glBindBuffer(GL_ARRAY_BUFFER, singleFaceVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, facePos);
	model = glm::scale(model, glm::vec3(0.4f));
	shader.setVec4("uObjColor", glm::vec4(faceColor, 0.5f));
	shader.setMat4("model", model);
	for (int j = 0; j < 3; ++j)
	{
		shader.setFloat("pTransmission[" + to_string(j) + "]", 0.0);
	}
	glBindVertexArray(singleFaceVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned int texShowVAO = 0;
unsigned int texShowVBO;

void initTexShowVAO()
{
	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
	};
	/*float quadVertices[] = {
		// positions        // texture Coords
		0.0f,  1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		 1.0f, 0.0f, 1.0f, 0.0f,
	};*/
	glGenVertexArrays(1, &texShowVAO);
	glGenBuffers(1, &texShowVBO);
	glBindVertexArray(texShowVAO);
	glBindBuffer(GL_ARRAY_BUFFER, texShowVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void DisplayFramebufferTexture(GLuint textureID)
{
	if (texShowVAO == 0)
	{
		initTexShowVAO();
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBindVertexArray(texShowVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void DisplayFramebufferTextureArray(GLuint textureID)
{
	if (texShowVAO == 0)
	{
		initTexShowVAO();
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

	glBindVertexArray(texShowVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void DisplayFramebufferTextureMsaa(GLuint textureID)
{
	if (texShowVAO == 0)
	{
		initTexShowVAO();
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);

	glBindVertexArray(texShowVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void renderPlane(const Shader& shader)
{
	if (planeVAO == 0)
	{
		// set up vertex data (and buffer(s)) and configure vertex attributes
		float planeVertices[] = {
			// positions            // normals         // texcoords
			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
		};
		// plane VAO
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}
	// floor
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0));
	model = glm::scale(model, glm::vec3(0.4f));
	shader.setMat4("model", model);
	for (int j = 0; j < 3; ++j)
	{
		shader.setFloat("pTransmission[" + to_string(j) + "]", 0.0);
	}
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

}

//不能使用几何着色器，在一个pass中渲染三个分量的深度图
//因为每个分量需要不同的丢弃概率，但是不能通过几何着色器传递int值，int不可以插值
//后来查到了可以使用flat out/flat in来解决这个问题
//当渲染到深度图的时候，并不需要排序，这是因为深度图只记录离光源最近的片段
//不管这个片段是透明的还是不透明的，不管是先绘制还是后绘制，总之只保留最近的片段就OK了
void renderSceneToDepth(const Shader& shader)
{
	for (int i = 0; i < cubeSceneData.size(); ++i)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, get<0>(cubeSceneData[i]));
		model = glm::scale(model, get<1>(cubeSceneData[i]));
		shader.setMat4("model", model);
		for (int j = 0; j < 3; ++j)
		{
			shader.setFloat("pTransmission[" + to_string(j) + "]", get<3>(cubeSceneData[i])[j]);
		}
		renderCube();
	}
}


void renderScene(const Shader& shader)
{
	for (auto it = cubeSceneData.begin(); it != cubeSceneData.end(); it++)//从后到前就是从远到近
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, get<0>(*it));
		model = glm::scale(model, get<1>(*it));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", get<2>(*it));
		//glSampleCoverage(0.0, GL_FALSE);
		renderCube();

	}
}

void renderLight(const Shader& shader, glm::vec3 lightPos)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(0.02));
	shader.setMat4("model", model);
	renderCube();
}


unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------

void renderCube()
{
	// 顶点错误并不会使程序崩掉，只会让一些面显示的不完全
	//正面的顶点是顺时针的，背面的顶点是逆时针的,
	//点的位置(如左上右下等，指的都是点在那个面的位置)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			
			// front face
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
		
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);
}


void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
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
