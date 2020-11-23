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
void processInput(GLFWwindow *window);
void renderQuad();
void renderCube();
void renderLight(const Shader &shader, glm::vec3 lightPos);
void renderScene(const Shader &shader);
void renderSceneToDepth(const Shader &shader);
void renderPlane(const Shader &shader);
void renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec4& faceColor);
unsigned int loadTexture(char const * path);
void DisplayFramebufferTexture(GLuint textureID);//调试用的函数，显示纹理屏幕上
void DisplayFramebufferTextureArray(GLuint textureID);
void DisplayFramebufferTextureMsaa(GLuint textureID);
void DisplayFramebufferImage(GLuint textureID);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 736;//为了配平计算着色器

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));//11.0 与单面相应的摄像机位置
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 targetPos(0.0f, 0.0f, 0.0f);//测试view矩阵的up向量的初始值

unsigned int planeVAO;
unsigned int planeVBO;
unsigned int singleFaceVAO;
unsigned int singleFaceVBO;
unsigned int cubeVAO;
unsigned int cubeVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int lightProjVAO;
unsigned int lightProjVBO;

//分别是cube的位置，scale，color，每种分量透射的比例((1-透明度)*分量的颜色)
float thickness = 0.3f;
vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>> cubeSceneData
{
	make_tuple(glm::vec3(0.0f, 0.2f, 4.0f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(0.0, 1.0, 0.0, 0.5),glm::vec3(1.0, 0.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 3.2f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(1.0, 0.0, 0.0, 0.5),glm::vec3(0.0, 1.0, 1.0)),
};

//双100的情况下大概是140FPS
#define addModelNum 0
#define listLayer 10
//计算着色器的全局工作组大小
const struct
{
	GLuint num_groups_x;
	GLuint num_groups_y;
	GLuint num_groups_z;
}dispatch_params = {40,23,1};



int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };

	for (int i = 0; i < addModelNum; ++i)
	{
		cubeSceneData.push_back(make_tuple(glm::vec3(0.0f, 0.2f, 4.0f), glm::vec3(0.4, 0.4, thickness),
			glm::vec4(0.0, 1.0, 0.0, 0.5), glm::vec3(1.0, 0.0, 1.0)));
		cubeSceneData.push_back(make_tuple(glm::vec3(0.0f, 0.2f, 3.2f), glm::vec3(0.4, 0.4, thickness),
			glm::vec4(1.0, 0.0, 0.0, 0.5), glm::vec3(0.0, 1.0, 1.0)));
	}
	
	//这个帧缓冲，在排序的时候，保存每一层的颜色和深度，然后将深度复制到compareMap中。将颜色复制到colorTexSet中
	//并且颜色纹理为支持多重采样的,正好它需要复制，解决了多重采样纹理无法直接在着色器中使用的问题
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



	//定义shader
	const std::string pathstr = "./shaders/";
	Shader quadShader((pathstr + "debug_quad.vs").c_str(), (pathstr + "debug_quad.fs").c_str());
	Shader texShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "texShow.fs").c_str());
	Shader imageShowShader((pathstr + "texShow.vs").c_str(), (pathstr + "imageShow.fs").c_str());
	//下面的是linklist的shader
	Shader linkListSampleShader((pathstr + "link_list_sample.vs").c_str(), (pathstr + "link_list_sample.fs").c_str());
	Shader linkListSampleSortShader((pathstr + "sortAndBlend.vs").c_str(), (pathstr + "sortAndBlend.fs").c_str());
	
	//Shader listSortShader((pathstr + "link_listSorted.comp").c_str());


	float camera_nearPlane = 0.1f, camera_farPlane = 10.0f;
	quadShader.use();
	quadShader.setFloat("near_plane", camera_nearPlane);
	quadShader.setFloat("far_plane", camera_farPlane);

	linkListSampleShader.use();
	GLuint64 maxListNodeNum = SCR_WIDTH * SCR_HEIGHT * listLayer;
	GLuint64 nodeSize = sizeof(glm::uvec4);
	GLuint64 ssboNodeSize = sizeof(glm::uvec4) + sizeof(glm::uvec2);
	linkListSampleShader.setInt("uMaxListNode", maxListNodeNum);

	//绘制透明物体的时候一定要排序，1.绘制所有的不透明物体。2.将透明的物体排序，再从远到近的渲染
	//因为深度测试的原因，如果不按照从远到近的顺序绘制的话，因为深度写入的原因，近的透明的物体
	//的深度会写入到深度缓冲中，然后远处的在这个透明物体的后面的透明物体就会因为深度测试被丢弃
	//因此只能从远到近的绘制

	//缓冲用来保存原子计数器，这个原子计数器给了每个片段唯一的序号标识
	unsigned int atomicBuffer;
	glGenBuffers(1, &atomicBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);//这个buffer的绑定点为0
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), nullptr,GL_DYNAMIC_COPY);//经常性的复制
	const int one = 1;
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), &one);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	

	unsigned int headPtrImage;//头节点表
	glGenTextures(1, &headPtrImage);
	glBindTexture(GL_TEXTURE_2D, headPtrImage);
	//因为头节点表中存储的是所有节点的index，所以可能会很大，因此不能使用GL_UNSIGNED_BYTE
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, SCR_WIDTH, SCR_HEIGHT, 0, GL_R32UI, GL_UNSIGNED_INT, NULL);
	//只能使用glTexStorage2D
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);//这里的level只能填1
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(1, headPtrImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);//绑定为image

	unsigned int finalColorImage;//最后的颜色结果,在使用compute shader时会使用
	glGenTextures(1, &finalColorImage);
	glBindTexture(GL_TEXTURE_2D, finalColorImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(2, finalColorImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

	unsigned int lockImage;//用来测试原子操作的，但是证明不得行
	glGenTextures(1, &lockImage);
	glBindTexture(GL_TEXTURE_2D, lockImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, SCR_WIDTH, SCR_HEIGHT);//这里的level只能填1
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindImageTexture(3, lockImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);//绑定为image

	//使用PBO初始化头节点表
	GLuint headPtrInitPBO;
	glGenBuffers(1, &headPtrInitPBO);
	glBindBuffer(GL_ARRAY_BUFFER , headPtrInitPBO);//texture data source
	//glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(UINT32), NULL, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH * SCR_HEIGHT * sizeof(GL_RGBA), NULL, GL_STATIC_DRAW);
	void* initData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);//只需要写入这个PBO
	memset(initData, 0, SCR_WIDTH* SCR_HEIGHT * sizeof(GL_RGBA));
	//memset(initData, 0, SCR_WIDTH * SCR_HEIGHT * sizeof(UINT32));//memset最好初始化为0，如果初始化为其它值，并不会获得期望的结果
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//GLuint finalColorInitPBO;
	//glGenBuffers(1, &finalColorInitPBO);
	//glBindBuffer(GL_ARRAY_BUFFER, finalColorInitPBO);
	//glBufferData(GL_ARRAY_BUFFER, SCR_WIDTH* SCR_HEIGHT * sizeof(GL_RGBA32F), NULL, GL_STATIC_DRAW);
	//void* initData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	//memset(initData, 0, SCR_WIDTH* SCR_HEIGHT * sizeof(GL_RGBA32F));
	//glUnmapBuffer(GL_ARRAY_BUFFER);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	unsigned int linkListBuffer;//存储所有片段
	glGenBuffers(1, &linkListBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, linkListBuffer);
	glBufferData(GL_TEXTURE_BUFFER, maxListNodeNum* nodeSize, NULL, GL_DYNAMIC_COPY);
	unsigned int linkListTex;//这个image在shader中的内存是imageBuffer
	glGenTextures(1, &linkListTex);
	glBindTexture(GL_TEXTURE_BUFFER, linkListTex);
	glTexBuffer(GL_TEXTURE_BUFFER,GL_RGBA32UI, linkListBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindImageTexture(0, linkListTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	//怎么把uimage2D和texBuffer传递到shader中呢？
	//我们先看image2D，它使用glBindImageTexture来指定绑定点，那么所有的shader都可以使用这个绑定点来访问这张image吗？
	//对于texBuffer肯定可以这样使用，因为类似于ubo，只要在着色器中使用layout(binding=0)就可以了

	while (!glfwWindowShouldClose(window))
	{
		//初始化原子计数器为1
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), &one);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		//初始化头节点image的数据为0
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, headPtrInitPBO);
		glBindTexture(GL_TEXTURE_2D, headPtrImage);
		//texSub/tex都是用来指定纹理数据的，当pixel_unpack_buffer不为空的时候，就会将缓冲中的数据复制到image中
		//texSub 只会更新纹理中的数据，而不用销毁内存重新申请
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		//glBindTexture(GL_TEXTURE_2D, finalColorImage);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		//glBindTexture(GL_TEXTURE_2D, lockImage);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		//注意这里只能使用GL_UNSIGNED_BYTE来初始化finalColorImage
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera_nearPlane, camera_farPlane);
		glClearColor(0.10f, 0.10f, 0.10f, 0.0f);
	
		if (true)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, singleColorFBO);
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			linkListSampleShader.use();
			linkListSampleShader.setMat4("projection", cameraProjection);
			linkListSampleShader.setMat4("view", cameraView);
			linkListSampleShader.setVec4("uObjColor", glm::vec4(0.0, 0.0, 1.0, 1.0));
			renderPlane(linkListSampleShader);
			//不透明的物体，写入到深度缓冲中
			//renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 3.3f), glm::vec4(0.1, 0.1, 0.1, 1.0));
			glDepthMask(GL_FALSE);
			renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 4.0f), glm::vec4(0.0, 1.0, 0.0, 0.5));
			renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 3.5f), glm::vec4(1.0, 0.0, 0.0, 0.4));
			renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 3.0f), glm::vec4(0.0, 0.0, 1.0, 0.3));
			renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 2.5f), glm::vec4(1.0, 1.0, 0.0, 0.2));
			renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 2.0f), glm::vec4(1.0, 0.0, 1.0, 0.1));
			renderSingFace(linkListSampleShader, glm::vec3(0.0f, 0.2f, 1.5f), glm::vec4(0.0, 1.0, 1.0, 0.6));
			//renderScene(linkListSampleShader);
			glDepthMask(GL_TRUE);

			//如果有两个shader，他们分别是A和B，那么本来你的执行逻辑的glUseProgram(A)->Draw->glUseProgram(B)->Draw。
			//此时A和B都往同一个shader storage block里写数据。 此时有的系统不一定保证A写完才会被B去写。
			//所以如果你把调用逻辑改成
			//glUseProgram(A)->Draw->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)->glUseProgram(B)->Draw，. 
			//这就可以保证写操作的先后顺序了。
			//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);//他妈的就是这句话让我改了两周
			glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);//可以让等待的区域更小
			glBindFramebuffer(GL_FRAMEBUFFER, singleColorFBO);//将最后的颜色输出到这个帧缓冲中
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			linkListSampleSortShader.use();
			renderQuad();
		}

		//使用buffer来发送全局工作组大小数据，这样就可以使用程序化生成的工作组大小
		//listSortShader.use();
		//glDispatchCompute(40, 23, 1);
		//unsigned int dispatchBuffer;
		//glGenBuffers(1, &dispatchBuffer);
		//glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatchBuffer);
		//glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(dispatch_params), &dispatch_params, GL_STATIC_DRAW);
		//glDispatchComputeIndirect(0);//指定offset
		//glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);

		//可视化
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quadShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, singleTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, finalColorImage);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, headPtrImage);
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
	glDeleteFramebuffers(1, &singleColorFBO);

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
	glBindVertexArray(texShowVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
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

void renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec4& faceColor)
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
	shader.setVec4("uObjColor", faceColor);
	shader.setMat4("model", model);
	glBindVertexArray(singleFaceVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


void renderPlane(const Shader &shader)
{
	if (planeVAO == 0)
	{
		// set up vertex data (and buffer(s)) and configure vertex attributes
		float planeVertices[] = {
			
			25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f,
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
	model = glm::scale(model, glm::vec3(1.4f));
	shader.setMat4("model", model);
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
	for (auto it = cubeSceneData.begin(); it != cubeSceneData.end(); it++)
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
	model = glm::scale(model, glm::vec3(0.02));
	shader.setMat4("model", model);
	renderCube();
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
