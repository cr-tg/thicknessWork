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
//使用了新的includes以后，之前对wingdi.h的引用就没有了，需要手动添加
//而添加wingdi.h之前必须要添加windows.h
#include <windows.h>
#include <wingdi.h>

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void renderQuad();
void renderCube();
void renderLight(const Shader &shader, glm::vec3 lightPos);
void renderScene(const Shader &shader);
void renderSceneToDepth(const Shader &shader);
void renderPlane(const Shader &shader);
void renderLightProjection(const Shader &shader, glm::mat4 model, float minX, float minY,
	float maxX, float maxY, float nearPlane, float farPlane);
unsigned int loadTexture(char const * path);
bool WriteBitmapFile(const char* filename, int wid, int hei, unsigned char* bitmapData);
void DisplayFramebufferTexture(GLuint textureID);
void DisplayFramebufferImage(GLuint textureID);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));//404
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//pcf
int pcfCount = 0;
bool pcfPressed = false;
bool usePcf = false;

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
unsigned int lightProjVAO;
unsigned int lightProjVBO;

//分别是cube的位置，scale，color，每种分量透射的比例((1-透明度)*分量的颜色)
vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>> cubeSceneData
{

	//场景1：2个圆柱和一个方块
	make_tuple(glm::vec3(2.0f, 0.0f, 1.0),glm::vec3(0.2, 2.0, 0.2),
	glm::vec4(1.0, 0.0, 0.0, 0.4),glm::vec3(0.6, 0.0, 0.0)),
	//glm::vec4(1.0, 0.8, 0.6, 0.4),glm::vec3(0.6, 0.48, 0.36)), 测试颜色
	//make_tuple(glm::vec3(1.0f, 0.0f, 0.0),glm::vec3(0.2, 2.0, 0.2),
	//glm::vec4(0.0, 1.0, 0.0, 0.4),glm::vec3(0.0, 1.0, 0.0)),
	//make_tuple(glm::vec3(3.5f, 0.0f, 0.5),glm::vec3(0.5, 0.3, 0.5),
	//glm::vec4(0.0, 0.0, 1.0, 0.4),glm::vec3(0.0, 0.0, 1.0)),
	//不透明的物体
	//make_tuple(glm::vec3(1.5f, 1.0f, 0.5),glm::vec3(0.2, 0.1, 0.2),
	//glm::vec4(0.0, 0.0, 0.0, 1.0),glm::vec3(0.0, 0.0, 0.0))
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

};

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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//	glfwWindowHint(GLFW_SAMPLES, 4);
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


	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_ALWAYS);
	//glDepthFunc(GL_GREATER);
	//glDepthFunc(GL_LESS);
//	glEnable(GL_MULTISAMPLE);
	//创建一个帧缓冲，使用一个深度的纹理数组来保存所有层的深度
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	//不能把深度信息和color信息放到同一个帧缓冲中，因为帧缓冲中总是会自动记录深度信息
	//因此在使用第二个着色器渲染的时候，深度信息会改变
	unsigned int depthFBO;
	glGenFramebuffers(1, &depthFBO);
	unsigned int msaaColorFBO;
	glGenFramebuffers(1, &msaaColorFBO);
	unsigned int interMediateFBO;//将离屏的msaa传到这个帧缓冲中
	glGenFramebuffers(1, &interMediateFBO);

	// create depth texture
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

	int offscreenSample = 4;
	unsigned int msaaColorTex;
	glGenTextures(1, &msaaColorTex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaColorTex);
	//颜色缓冲的大小必须是和屏幕一致的，否则屏幕的一块是黑的，没有被渲染
	//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, offscreenSample, GL_RGB, SHADOW_WIDTH, SHADOW_HEIGHT, GL_TRUE);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, offscreenSample, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 0);
	//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, borderColor);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, offscreenSample, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapArray, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, msaaColorFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaaColorTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int colorTex;
	glGenTextures(1, &colorTex);
	glBindTexture(GL_TEXTURE_2D, colorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "interMediateFBO not complete！" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int testTex;
	glGenTextures(1, &testTex);
	glBindTexture(GL_TEXTURE_2D, testTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	unsigned int testBuffer0;
	glGenBuffers(1, &testBuffer0);
	glBindBuffer(GL_ARRAY_BUFFER, testBuffer0);
	glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * 4, NULL, GL_STREAM_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	unsigned int testImage0;
	glGenTextures(1, &testImage0);
	glBindTexture(GL_TEXTURE_2D, testImage0);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(0, testImage0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);//绑定为image

	unsigned int ssbo;//存储所有片段的ssbo，并且链接到着色器的0号位置
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, SCR_WIDTH * SCR_HEIGHT * 4, NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);//Read-write storage for shaders
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	unsigned int testImage1;
	glGenTextures(1, &testImage1);
	glBindTexture(GL_TEXTURE_2D, testImage1);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(1, testImage1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	GLuint testBuffer1;
	glGenBuffers(1, &testBuffer1);
	glBindBuffer(GL_ARRAY_BUFFER, testBuffer1);
	glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * 4, NULL, GL_STATIC_DRAW);
	void* initData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memset(initData, 0xF, 4);//初始化为0xFFFFFFFFFFF...
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//定义shader
	const std::string pathstr = "./shaders/";
	Shader depthShader((pathstr + "shadow_depth.vs").c_str(), (pathstr + "shadow_depth.fs").c_str(),
		(pathstr + "shadow_depth.geom").c_str());
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader shadowShader((pathstr + "simpleShadow.vs").c_str(), (pathstr + "simpleShadow.fs").c_str());
	Shader texShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "texShow.fs").c_str());
	Shader imageShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "imageShow.fs").c_str());

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//定义光源信息
	glm::vec3 lightPos(-2.0f, 1.0f, -1.0f);
	//glm::vec3 lightPos(-6.0f, 4.0f, -1.0f);
	glm::mat4 lightProjection, lightView;
	float near_plane = 2.0f, far_plane = 10.5f;
	float camera_nearPlane = 0.1f, camera_farPlane = 100.0f;
	lightProjection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, near_plane, far_plane);
	//不应该使用透视投影，因为这是用在点光源阴影的生成中的，需要万向投影贴图
	//lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane);
	lightView = glm::lookAt(lightPos, targetPos, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 inverseInitLightView = glm::inverse(lightView);
	depthShader.use();
	depthShader.setMat4("projection", lightProjection);
	depthShader.setMat4("view", lightView);
	quadShader.use();
	quadShader.setFloat("near_plane", near_plane);
	quadShader.setFloat("far_plane", far_plane);
	quadShader.setInt("testMap", 0);
	quadShader.setInt("colorTex", 1);
	shadowShader.use();
	shadowShader.setMat4("lightVPMatrix", lightProjection * lightView);
	shadowShader.setInt("uShadowMapLinear", 0);
	shadowShader.setInt("uWoodTexture", 1);
	shadowShader.setInt("testMap", 2);
	for (int i = 0; i < offsetVec.size(); ++i)
	{
		shadowShader.setVec2("offsetVec[" + to_string(i) + "]",offsetVec[i]);
	}
	//绘制透明物体的时候一定要排序，1.绘制所有的不透明物体。2.将透明的物体排序，再从远到近的渲染
	//因为深度测试的原因，如果不按照从远到近的顺序绘制的话，因为深度写入的原因，近的透明的物体
	//的深度会写入到深度缓冲中，然后远处的在这个透明物体的后面的透明物体就会因为深度测试被丢弃
	//因此只能从远到近的绘制
	int lastPcfCount = -1;
	//cout << "300:" << glGetError() << endl;
	while (!glfwWindowShouldClose(window))
	{
		static int tt = 1;
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER,depthFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		depthShader.use();
		renderPlane(depthShader);
		renderSceneToDepth(depthShader);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, msaaColorFBO);
		//glBindFramebuffer(GL_FRAMEBUFFER, interMediateFBO);//interMediateFBO
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shadowShader.use();
		shadowShader.setMat4("projection", cameraProjection);
		shadowShader.setMat4("view", cameraView);
		shadowShader.setVec3("lightColor", glm::vec3(0.3f));
		shadowShader.setVec3("lightPos", lightPos);
		shadowShader.setVec3("viewPos", camera.Position);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, depthMapArray);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D_ARRAY, depthMapArray);
		shadowShader.setBool("uIsPlan", true);
		renderPlane(shadowShader);
		shadowShader.setBool("uIsLight", true);
		renderLight(shadowShader, lightPos);
		shadowShader.setBool("uIsLight", false);
		shadowShader.setBool("uIsPlan", false);
		shadowShader.setBool("usePcf", usePcf);
		//普通的pcf采样
		shadowShader.setInt("uPcfCount", pcfCount);
		if (lastPcfCount != pcfCount && usePcf)
		{
			lastPcfCount = pcfCount;
			cout << "pcfCount:" << pcfCount << endl;
		}
		if (lastPcfCount != pcfCount && !usePcf)
		{
			lastPcfCount = pcfCount;
			cout << "notUsePcf:" << endl;
		}
		//论文中提供的间隔点作为pcf采样,放在了渲染循环外面
		renderScene(shadowShader);
		glDisable(GL_BLEND);
		//blit msaa buffer to normal buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaColorFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, interMediateFBO);
		glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, 
			GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		quadShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, depthMapArray);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, colorTex);
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
	glDeleteFramebuffers(1, &depthFBO);
	glDeleteFramebuffers(1, &msaaColorFBO);

	glfwTerminate();
	return 0;
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

void DisplayFramebufferImage(GLuint textureID)
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

void DisplayFramebufferTexture(GLuint textureID)
{
	if (texShowVAO == 0)
	{
		initTexShowVAO();
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	//glActiveTexture(GL_TEXTURE1);
	glBindVertexArray(texShowVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

bool WriteBitmapFile(const char* filename, int wid, int hei, unsigned char* bitmapData)
{
	int width = wid;
	int height = hei;

	BITMAPFILEHEADER bitmapFileHeader;
	memset(&bitmapFileHeader, 0, sizeof(BITMAPFILEHEADER));
	bitmapFileHeader.bfSize = sizeof(BITMAPFILEHEADER);
	bitmapFileHeader.bfType = 0x4d42;	//BM
	bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//填充BITMAPINFOHEADER
	BITMAPINFOHEADER bitmapInfoHeader;
	memset(&bitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfoHeader.biWidth = width;
	bitmapInfoHeader.biHeight = height;
	bitmapInfoHeader.biPlanes = 1;
	bitmapInfoHeader.biBitCount = 24;
	bitmapInfoHeader.biCompression = BI_RGB;
	bitmapInfoHeader.biSizeImage = width * abs(height) * 3;

	//////////////////////////////////////////////////////////////////////////
	FILE* filePtr;
	unsigned char tempRGB;
	int imageIdx;

	//swap R B
	for (imageIdx = 0; imageIdx < bitmapInfoHeader.biSizeImage; imageIdx += 3)
	{
		tempRGB = bitmapData[imageIdx];
		bitmapData[imageIdx] = bitmapData[imageIdx + 2];
		bitmapData[imageIdx + 2] = tempRGB;
	}

	filePtr = fopen(filename, "wb");
	if (NULL == filePtr)
	{
		return false;
	}

	fwrite(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

	fwrite(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	fwrite(bitmapData, bitmapInfoHeader.biSizeImage, 1, filePtr);

	fclose(filePtr);
	return true;
}

void renderPlane(const Shader &shader)
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
void renderSceneToDepth(const Shader &shader)
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


void renderScene(const Shader &shader)
{
	//观察阴影区
	//将半透明的物体排序并保存到这个map中，因为map会根据key值自动排序，
	//因此只需要将距离作为key值进行排序即可
	map<float, tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>> sortedMap;
	glm::vec3 cameraPos = camera.Position;
	for (int i = 0; i < cubeSceneData.size(); ++i)
	{
		float distance = glm::length(cameraPos - get<0>(cubeSceneData[i]));
		sortedMap[distance] = cubeSceneData[i];
	}
	/*for (auto it = sortedMap.rbegin();it!=sortedMap.rend();it++)//从后到前就是从远到近
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, get<0>(it->second));
		model = glm::scale(model, get<1>(it->second));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", get<2>(it->second));
		renderCube();
	}*/
	for (auto it = cubeSceneData.begin(); it != cubeSceneData.end(); it++)//按照给定的顺序渲染
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, get<0>(*it));
		model = glm::scale(model, get<1>(*it));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", get<2>(*it));
		renderCube();
	}
}

void renderLight(const Shader &shader,glm::vec3 lightPos)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(0.2));
	shader.setMat4("model", model);
	renderCube();
}

void renderLightProjection(const Shader &shader, glm::mat4 model, float minX, float minY,
	float maxX, float maxY, float nearPlane, float farPlane)
{
	shader.setMat4("model", model);
	glm::vec3 frustum_near_leftup(minX, minY, -nearPlane);
	glm::vec3 frustum_near_rightup(maxX, minY, -nearPlane);
	glm::vec3 frustum_near_leftdown(minX, maxY, -nearPlane);
	glm::vec3 frustum_near_rightdown(maxX, maxY, -nearPlane);
	glm::vec3 frustum_far_leftup(minX, maxY, -farPlane);
	glm::vec3 frustum_far_rightup(maxX, maxY, -farPlane);
	glm::vec3 frustum_far_leftdown(minX, minY, -farPlane);
	glm::vec3 frustum_far_rightdown(maxX, minY, -farPlane);

	if (lightProjVAO == 0)
	{
		float vertices[] = {
			// back face
			minX, minY, -farPlane,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 maxX,  maxY, -farPlane,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 maxX, minY, -farPlane,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 maxX,  maxY, -farPlane,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			minX, minY, -farPlane,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			minX,  maxY, -farPlane,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			minX, minY,  -nearPlane,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 maxX, minY,  -nearPlane,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 maxX,  maxY,  -nearPlane,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 maxX,  maxY,  -nearPlane,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			minX,  maxY,  -nearPlane,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			minX, minY,  -nearPlane,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			minX,  maxY,  -nearPlane, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			minX,  maxY, -farPlane, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			minX, minY, -farPlane, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			minX, minY, -farPlane, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			minX, minY,  -nearPlane, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			minX,  maxY,  -nearPlane, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 maxX,  maxY,  -nearPlane,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 maxX,  minY, -farPlane,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 maxX,  maxY, -farPlane,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 maxX,  minY, -farPlane,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 maxX,  maxY,  -nearPlane,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 maxX,  minY,  -nearPlane,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left    
			// bottom face
			minX,  minY, -farPlane,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 maxX,  minY, -farPlane,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 maxX, minY,  -nearPlane,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 maxX, minY,  -nearPlane,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			minX, minY,  -nearPlane,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			minX,  minY, -farPlane,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			minX,  maxY, -farPlane,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 maxX, maxY , -nearPlane,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 maxX,  maxY, -farPlane,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			maxX, maxY,  -nearPlane,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			minX,  maxY, -farPlane,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			minX, maxY,  -nearPlane,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left   
		};
		glGenVertexArrays(1, &lightProjVAO);
		glGenBuffers(1, &lightProjVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, lightProjVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(lightProjVAO);
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
	glBindVertexArray(lightProjVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}


unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
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
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
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

void processInput(GLFWwindow *window)
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

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !pcfPressed)
	{
		pcfCount++;
		if (pcfCount == 5)
		{
			usePcf = false;
			pcfCount = 0;
		}
		else
		{
			usePcf = true;
		}
		pcfPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE)
	{
		pcfPressed = false;
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
