#pragma once
#include <string>
#include "RenderObject.h"
#include "Light.h"

typedef std::vector<RenderObject*> Mesh;

class Model
{
public:
	Model();
	~Model();
	void Finalize();

	void CreateModel(std::string textureName, const void* pVertexData, size_t vertexCount, const void* pIndexData, size_t indexCount, GraphicSystem* pGraphicSystem);
	void CreateModel(std::string fileName, std::string textureRoot, GraphicSystem* pGraphicSystem);
	
	void Draw(VkCommandBuffer commandBuffer, uint32_t index);

	void Update(uint32_t index);

	void SetTranslate(glm::vec3 translateVec)
	{
		m_Translate = translateVec;
		//m_Transform.modelMtx = glm::translate(glm::mat4(1.0f), glm::vec3(0, m_Offset, 0));
		//m_Transform.modelMtx = glm::rotate(m_Transform.modelMtx, /*time * */glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)); //glm::mat4(1.0);
	}
	void SetRotate(glm::quat rotateQuat)
	{
		//m_Transform.modelMtx = glm::rotate(m_Transform.modelMtx, /*time * */glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)); //glm::mat4(1.0);
		m_Rotate = rotateQuat;
	}
	void SetScale(glm::vec3 scale)
	{
		m_Scale = scale;
	}

private:
	VkDevice m_Device;
	VkShaderModule m_VsShaderModule;
	VkShaderModule m_FsShaderModule;
	std::vector<Mesh> meshes;
	std::vector<Light*> lights;

	glm::vec3 m_Translate;
	glm::vec3 m_Scale;
	glm::quat m_Rotate;
	glm::mat4 m_WorldTransform;

};

