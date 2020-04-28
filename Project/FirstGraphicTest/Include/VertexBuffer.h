#pragma once
#include "Helper.h"
#include "GraphicSystem.h"

enum VertexAttributeFlag
{
	POSION = 1,
	NORMAL = 1 << 1,
	TANGENT = 1 << 2,
	TEXCOORD = 1 << 3,
};

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();

	void Finalize();

	void CreateVertexBuffer(GraphicSystem* pGraphicSystem, const void* pData, size_t dataSize);
	void CreateIndexBuffer(GraphicSystem* pGraphicSystem, const void* pData, size_t dataSize, VkIndexType indexType);

	VkBuffer* GetVertexBuffer()
	{
		return &m_VertexBuffer;
	}
	VkBuffer* GetIndexBuffer()
	{
		return &m_IndexBuffer;
	}
	VkIndexType GetIndexType()
	{
		return m_IndexType;
	}
private:
	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;

	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;

	VkIndexType m_IndexType;

	VkDevice m_Device;
};