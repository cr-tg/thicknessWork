#pragma once
#include "setting.h"
#include <learnopengl/shader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <tuple>

using std::vector;
using std::tuple;
using std::make_tuple;

//这里因为3个实验都需要同样的场景，因此直接把场景放到工具库里了，以免每次都去复制一遍。

//分别是cube的位置，scale，color，每种分量透射的比例((1-透明度)*分量的颜色)
float thickness = 0.2f;//注意这个厚度不是指的cube的厚度，而是cube中心离两侧边缘的距离
vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>> cubeSceneData
{
	//make_tuple(glm::vec3(0.0f, 0.2f, 4.0f),glm::vec3(0.4, 0.4, thickness),
	//glm::vec4(0.0, 1.0, 0.0, 0.5),glm::vec3(1.0, 0.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 3.2f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(1.0, 0.0, 0.0, 0.5),glm::vec3(0.0, 1.0, 1.0)),
	make_tuple(glm::vec3(0.0f, 0.2f, 2.4f),glm::vec3(0.4, 0.4, thickness),
	glm::vec4(0.0, 0.0, 1.0, 0.5),glm::vec3(1.0, 1.0, 0.0)),
};

float radius = 0.2f;//pos scale color
vector<tuple<glm::vec3, glm::vec3, glm::vec4>> sphereSceneData
{
	make_tuple(glm::vec3(0.0f, 0.2f, 4.0f),glm::vec3(radius),
	glm::vec4(0.0, 1.0, 0.0, 0.5)),
	make_tuple(glm::vec3(0.0f, 0.2f, 3.2f),glm::vec3(radius),
	glm::vec4(1.0, 0.0, 0.0, 0.5)),
	make_tuple(glm::vec3(0.0f, 0.2f, 2.4f),glm::vec3(radius),
	glm::vec4(0.0, 0.0, 1.0, 0.5)),
};


//最开始想做成工具类，只包含static函数，没有成员变量
//但是因为绘制必须要有VAO,VBO，因此还是做成单例类更好
class CRender
{
public:
	static GLTOOLSDLL_API CRender* getOrCreateWindowInstance()
	{
		if (m_pRender == nullptr)
		{
			m_pRender = new CRender();
		}
		return m_pRender;
	}

	GLTOOLSDLL_API CRender* getRender() const
	{
		return m_pRender;
	}
	//注意所有的shader 都是用的const&来传递的，因此不能使用const shader.use，所以必须在调用这些render之前，
	//先使用shader.use()
	void GLTOOLSDLL_API renderTriangle(const Shader& shader);
	void GLTOOLSDLL_API renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec4& faceColor);
	void GLTOOLSDLL_API renderPlane(const Shader& shader);
	void GLTOOLSDLL_API renderCubeSetFromData(const Shader& shader, const 
		vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>>& sceneData);
	void GLTOOLSDLL_API renderSphereSetFromData(const Shader& shader, const
		vector<tuple<glm::vec3, glm::vec3, glm::vec4>>& sphereData);

	void GLTOOLSDLL_API renderCubeSet(const Shader& shader);
	void GLTOOLSDLL_API renderSphereSet(const Shader& shader);

	//因为这里只是将场景绘制到一个quad上，至于是怎么绘制的，由shader决定
	//最简单的就是给定一张texture，然后采样
	void GLTOOLSDLL_API renderQuad();//因为只是将texture展示出来绘制一个quad，因此不需要任何的shader来传递uniform变量
	void GLTOOLSDLL_API renderSingleCube(const Shader& shader, const glm::vec3& lightPos, const glm::vec3& scale,
		const glm::vec4& color);

	void GLTOOLSDLL_API renderSphere(const Shader& shader, const glm::vec3& pos, const glm::vec3& scale,
		const glm::vec4& color);

	void GLTOOLSDLL_API renderTours(const Shader& shader);
	//所有的工程都使用这个函数来计算误差率，免得每个工程都去改一遍代码
	void GLTOOLSDLL_API testErrorRateScene(const Shader& shader);

private:
	CRender();
	void __initSingleFaceVAO();
	void __initPlaneVAO();

	void __renderCube();//这个函数是在给定了model和shader以后，绘制一个局部的正方体的，具体的形状与位置在这个函数外指定
	void __initCubeVAO();
	void __initQuadVAO();
	void __initTriangleVAO();

	void __renderSphere();
	void __initSphereVAO();
	void __renderTours();
	void __initToursVAO(int sides, int cs_sides, float radius, float cs_radius);//圆环

	static CRender* m_pRender;
	unsigned int m_SingleFaceVAO = 0;//必须初始化为0，否则可能会是很大的值
	unsigned int m_SingleFaceVBO = 0;
	unsigned int m_PlaneVAO = 0;
	unsigned int m_PlaneVBO = 0;
	unsigned int cubeVAO = 0;
	unsigned int cubeVBO = 0;
	unsigned int quadVAO = 0;
	unsigned int quadVBO = 0;
	unsigned int triangleVAO = 0;
	unsigned int trianlgeVBO = 0;
	unsigned int sphereVAO = 0;
	unsigned int sphereIndexCount = 0;
	unsigned int toursVAO = 0;
	unsigned int toursIndexCount = 0;
};

