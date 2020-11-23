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

//������Ϊ3��ʵ�鶼��Ҫͬ���ĳ��������ֱ�Ӱѳ����ŵ����߿����ˣ�����ÿ�ζ�ȥ����һ�顣

//�ֱ���cube��λ�ã�scale��color��ÿ�ַ���͸��ı���((1-͸����)*��������ɫ)
float thickness = 0.2f;//ע�������Ȳ���ָ��cube�ĺ�ȣ�����cube�����������Ե�ľ���
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


//�ʼ�����ɹ����ֻ࣬����static������û�г�Ա����
//������Ϊ���Ʊ���Ҫ��VAO,VBO����˻������ɵ��������
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
	//ע�����е�shader �����õ�const&�����ݵģ���˲���ʹ��const shader.use�����Ա����ڵ�����Щrender֮ǰ��
	//��ʹ��shader.use()
	void GLTOOLSDLL_API renderTriangle(const Shader& shader);
	void GLTOOLSDLL_API renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec4& faceColor);
	void GLTOOLSDLL_API renderPlane(const Shader& shader);
	void GLTOOLSDLL_API renderCubeSetFromData(const Shader& shader, const 
		vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>>& sceneData);
	void GLTOOLSDLL_API renderSphereSetFromData(const Shader& shader, const
		vector<tuple<glm::vec3, glm::vec3, glm::vec4>>& sphereData);

	void GLTOOLSDLL_API renderCubeSet(const Shader& shader);
	void GLTOOLSDLL_API renderSphereSet(const Shader& shader);

	//��Ϊ����ֻ�ǽ��������Ƶ�һ��quad�ϣ���������ô���Ƶģ���shader����
	//��򵥵ľ��Ǹ���һ��texture��Ȼ�����
	void GLTOOLSDLL_API renderQuad();//��Ϊֻ�ǽ�textureչʾ��������һ��quad����˲���Ҫ�κε�shader������uniform����
	void GLTOOLSDLL_API renderSingleCube(const Shader& shader, const glm::vec3& lightPos, const glm::vec3& scale,
		const glm::vec4& color);

	void GLTOOLSDLL_API renderSphere(const Shader& shader, const glm::vec3& pos, const glm::vec3& scale,
		const glm::vec4& color);

	void GLTOOLSDLL_API renderTours(const Shader& shader);
	//���еĹ��̶�ʹ�������������������ʣ����ÿ�����̶�ȥ��һ�����
	void GLTOOLSDLL_API testErrorRateScene(const Shader& shader);

private:
	CRender();
	void __initSingleFaceVAO();
	void __initPlaneVAO();

	void __renderCube();//����������ڸ�����model��shader�Ժ󣬻���һ���ֲ���������ģ��������״��λ�������������ָ��
	void __initCubeVAO();
	void __initQuadVAO();
	void __initTriangleVAO();

	void __renderSphere();
	void __initSphereVAO();
	void __renderTours();
	void __initToursVAO(int sides, int cs_sides, float radius, float cs_radius);//Բ��

	static CRender* m_pRender;
	unsigned int m_SingleFaceVAO = 0;//�����ʼ��Ϊ0��������ܻ��Ǻܴ��ֵ
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

