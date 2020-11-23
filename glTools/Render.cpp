#include "Render.h"

CRender* CRender::m_pRender;

CRender::CRender()
{
}

void CRender::renderTriangle(const Shader& shader)
{
	if (triangleVAO == 0)
	{
		__initTriangleVAO();
	}
	glBindVertexArray(triangleVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	//glDrawArrays(GL_TRIANGLES, 0, 6);
}

void CRender::renderSingFace(const Shader& shader, const glm::vec3& facePos, const glm::vec4& faceColor)
{
	if (m_SingleFaceVAO == 0)
	{
		__initSingleFaceVAO();
	}
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, facePos);
	model = glm::scale(model, glm::vec3(0.4f));
	shader.setVec4("uObjColor", faceColor);
	shader.setMat4("model", model);
	glBindVertexArray(m_SingleFaceVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void CRender::renderPlane(const Shader& shader)
{
	if (m_PlaneVAO == 0)
	{
		__initPlaneVAO();
	}
	// floor
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0));
	model = glm::scale(model, glm::vec3(1.4f));
	shader.setMat4("model", model);
	glBindVertexArray(m_PlaneVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void CRender::renderCubeSetFromData(const Shader& shader, const vector<tuple<glm::vec3, glm::vec3, glm::vec4, glm::vec3>>& sceneData)
{
	for (auto it = sceneData.begin(); it != sceneData.end(); it++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, std::get<0>(*it));
		model = glm::scale(model, std::get<1>(*it));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", std::get<2>(*it));
		__renderCube();
	}
}

void CRender::renderSphereSetFromData(const Shader& shader, const vector<tuple<glm::vec3, glm::vec3, glm::vec4>>& sphereData)
{
	for (auto it = sphereData.begin(); it != sphereData.end(); it++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, std::get<0>(*it));
		model = glm::scale(model, std::get<1>(*it));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", std::get<2>(*it));
		__renderSphere();
	}
}

void CRender::renderCubeSet(const Shader& shader)
{
	for (auto it = cubeSceneData.begin(); it != cubeSceneData.end(); it++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, std::get<0>(*it));
		model = glm::scale(model, std::get<1>(*it));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", std::get<2>(*it));
		__renderCube();
	}
}

void CRender::renderSphereSet(const Shader& shader)
{
	for (auto it = sphereSceneData.begin(); it != sphereSceneData.end(); it++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, std::get<0>(*it));
		model = glm::scale(model, std::get<1>(*it));
		shader.setMat4("model", model);
		shader.setVec4("uObjColor", std::get<2>(*it));
		__renderSphere();
	}
}

void CRender::renderQuad()
{
	if (quadVAO == 0)
	{
		__initQuadVAO();
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void CRender::renderSingleCube(const Shader& shader, const glm::vec3& lightPos, const glm::vec3& scale, const glm::vec4& color)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, scale);
	shader.setMat4("model", model);
	shader.setVec4("uObjColor", color);
	__renderCube();
}

void CRender::renderSphere(const Shader& shader, const glm::vec3& pos, const glm::vec3& scale, const glm::vec4& color)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, pos);
	model = glm::scale(model, scale);
	shader.setMat4("model", model);
	shader.setVec4("uObjColor", color);
	__renderSphere();
}

void CRender::renderTours(const Shader& shader)
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::vec3 tourPos = glm::vec3(0.0f, 0.2f, 4.0f);
	glm::vec3 tourScale = glm::vec3(1.0f);
	glm::vec4 tourColor = glm::vec4(0.0, 1.0, 0.0, 0.5);
	model = glm::translate(model, tourPos);
	model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
	model = glm::scale(model, tourScale);
	shader.setMat4("model", model);
	shader.setVec4("uObjColor", tourColor);
	__renderTours();
}

void CRender::testErrorRateScene(const Shader& shader)
{
	renderTours(shader);
	//renderSphereSet(shader);
}

void CRender::__initSingleFaceVAO()
{
	float planeVertices[] = {
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
	};
	glGenVertexArrays(1, &m_SingleFaceVAO);
	glGenBuffers(1, &m_SingleFaceVBO);
	glBindVertexArray(m_SingleFaceVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_SingleFaceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);
}

void CRender::__initPlaneVAO()
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
	glGenVertexArrays(1, &m_PlaneVAO);
	glGenBuffers(1, &m_PlaneVBO);
	glBindVertexArray(m_PlaneVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_PlaneVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);
}

void CRender::__renderCube()
{
	// 顶点错误并不会使程序崩掉，只会让一些面显示的不完全
	//正面的顶点是顺时针的，背面的顶点是逆时针的,
	//点的位置(如左上右下等，指的都是点在那个面的位置)
	if (cubeVAO == 0)
	{
		__initCubeVAO();
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void CRender::__initCubeVAO()
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

void CRender::__initQuadVAO()
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

void CRender::__initTriangleVAO()
{
	//float vertices[] = {
	// //  -1.0f, -1.0f, 0.0f, // leftdown  
	//	//1.0f, -1.0f, 0.0f, // rightdown
	// //  -1.0f,  1.0f, 0.0f,  // topleft   
	//	1.0f, -1.0f, 0.0f, // rightdown 
	//	1.0f,  1.0f, 0.0f, // rightup 
	//   -1.0f,  1.0f, 0.0f, // leftup 
	//};
	float vertices[] = {
   -1.0f, -1.0f, 0.0f, // left  
	1.0f, -1.0f, 0.0f, // right 
	0.0f,  1.0f, 0.0f  // top   
	};

	glGenVertexArrays(1, &triangleVAO);
	glGenBuffers(1, &trianlgeVBO);
	glBindVertexArray(triangleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, trianlgeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CRender::__renderSphere()
{
	if (sphereVAO == 0)
	{
		__initSphereVAO();
	}
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void CRender::__initSphereVAO()
{
	glGenVertexArrays(1, &sphereVAO);

	unsigned int vbo, ebo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uv;
	std::vector<glm::vec3> normals;
	std::vector<unsigned int> indices;

	const unsigned int X_SEGMENTS = 64;
	const unsigned int Y_SEGMENTS = 64;
	//const unsigned int X_SEGMENTS = 4;
	//const unsigned int Y_SEGMENTS = 2;
	const float PI = 3.14159265359;
	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			positions.push_back(glm::vec3(xPos, yPos, zPos));
			uv.push_back(glm::vec2(xSegment, ySegment));
			normals.push_back(glm::vec3(xPos, yPos, zPos));
		}
	}

	bool oddRow = false;
	for (int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (int x = 0; x <= X_SEGMENTS; ++x)
			{
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
	sphereIndexCount = indices.size();

	std::vector<float> data;
	for (int i = 0; i < positions.size(); ++i)
	{
		data.push_back(positions[i].x);
		data.push_back(positions[i].y);
		data.push_back(positions[i].z);
		if (uv.size() > 0)
		{
			data.push_back(uv[i].x);
			data.push_back(uv[i].y);
		}
		if (normals.size() > 0)
		{
			data.push_back(normals[i].x);
			data.push_back(normals[i].y);
			data.push_back(normals[i].z);
		}
	}
	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	float stride = (3 + 2 + 3) * sizeof(float);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
}

void CRender::__renderTours()
{
	if (toursVAO == 0)
	{
		int sides, cs_sides;
		float radius, cs_radius;
		sides = 1024;//sides就是环上的顶点数，3个就是个三角形
		//cs_sides为1时，只有三个不同的顶点，但是index在内外跳，比如041，但是04是同一个顶点，因此不能形成三角形
		//cs_sides为2时，有6个不同的但是Z值相同的顶点，此时index内外跳，就能形成三角形，此时的图形是三角圆环，但是厚度为0
		//cs_sides为3时，有12个不同的顶点，其中6个就是为2时的顶点，其余6个顶点是半径在内外半径之间的，然后z值在相反数上的位置
		cs_sides = 1024;
		radius = 1.0f;
		cs_radius = 0.1f; //内环半径是radius-cs_radius,外环半径是radius+cs_radius
		__initToursVAO(sides, cs_sides, radius, cs_radius);
	}
	glBindVertexArray(toursVAO);
	glDrawElements(GL_TRIANGLE_STRIP, toursIndexCount, GL_UNSIGNED_INT, 0);
}

typedef struct Model_t* Model;
struct Model_t
{
	int    vertex, index;
	float* vertices;
	float* tex;
	float* normals;
	uint16_t* indices;
};
void vecNormalize(float A[3])
{
	float len = 1 / sqrt(A[0] * A[0] + A[1] * A[1] + A[2] * A[2]);
	A[0] *= len;
	A[1] *= len;
	A[2] *= len;
}
#define M_PI 3.14159265359
#define D_TO_R     (M_PI/180)

void CRender::__initToursVAO(int sides, int cs_sides, float radius, float cs_radius)//cs开头的就是内环
{

	glGenVertexArrays(1, &toursVAO);

	unsigned int vbo, ebo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uv;
	std::vector<glm::vec3> normals;
	std::vector<unsigned int> indices;

	int numVertices = (sides + 1) * (cs_sides + 1);
	int numIndices = (2 * sides + 4) * cs_sides;

	//int angleincs = 360 / sides;
	//int cs_angleincs = 360 / cs_sides;
	float angleincs = 360.0f / (float)sides;
	float cs_angleincs = 360.0f / (float)cs_sides;
	float currentradius, zval;
	int nextrow;
	

	/* iterate cs_sides: inner ring */
	for (float j = 0.0f; j <= 360.0f; j += cs_angleincs)
	{
		currentradius = radius + (cs_radius * cos(j * D_TO_R));//cos返回的是double，cosf返回的是float
		zval = cs_radius * sin(j * D_TO_R);
		/* iterate sides: outer ring */
		for (float i = 0.0f; i <= 360.0f; i += angleincs)
		{
			float xPos = currentradius * cos(i * D_TO_R);
			float yPos = currentradius * sin(i * D_TO_R);
			float zPos = zval;

			float u = i / 360.;
			float v = 2. * j / 360 - 1;
			if (v < 0) v = -v;
			positions.push_back(glm::vec3(xPos, yPos, zPos));
			uv.push_back(glm::vec2(u, v));
		}
	}

	//std::cout << positions.size() << std::endl;
	/* compute normals: loops are swapped */
	int vertIdx = 0;
	for (float i = 0.0f, nextrow = (sides + 1) * 3; i <= 360.0f; i += angleincs, vertIdx += 1)
	{
		float xc = radius * cos(i * D_TO_R);
		float yc = radius * sin(i * D_TO_R);
		std::vector<float> norm;
		for (float j = 0.0f; j <= 360.0f; j += cs_angleincs)
		{
			float normX = positions[vertIdx].x - xc;
			float normY = positions[vertIdx].y - yc;
			float normZ = positions[vertIdx].z;
			norm.push_back(normX);
			norm.push_back(normY);
			norm.push_back(normZ);
			vecNormalize(norm.data());
			normals.push_back(glm::vec3(norm[0], norm[1], norm[2]));
		}
	}
	/* indices grouped by GL_TRIANGLE_STRIP, face oriented clock-wise */
	/* inner ring */
	for (int i = 0, nextrow = sides + 1; i < cs_sides; i++)
	{
		/* outer ring */
		for (int j = 0; j < sides; j++)
		{
			indices.push_back(i * nextrow + j);
			indices.push_back((i + 1) * nextrow + j);
		}
		//因为在末尾的时候，最后的两个三角形的index的顺序有问题，因此添加几个顶点，来让这个顺序正确
		indices.push_back(i * nextrow + sides);
		indices.push_back(i * nextrow + sides + nextrow);
		indices.push_back(i * nextrow + sides + nextrow);
		indices.push_back(i * nextrow + sides + nextrow);
	}
	toursIndexCount = indices.size();

	std::vector<float> data;
	for (int i = 0; i < positions.size(); ++i)
	{
		data.push_back(positions[i].x);
		data.push_back(positions[i].y);
		data.push_back(positions[i].z);
		if (uv.size() > 0)
		{
			data.push_back(uv[i].x);
			data.push_back(uv[i].y);
		}
		if (normals.size() > 0)
		{
			data.push_back(normals[i].x);
			data.push_back(normals[i].y);
			data.push_back(normals[i].z);
		}
	}

	glBindVertexArray(toursVAO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	float stride = (3 + 2 + 3) * sizeof(float);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
//	return torus;
}




Model modelGenTorus(int sides, int cs_sides, float radius, float cs_radius)
{
	
}

