#include "VertexBuffer.h"

VertexBuffer::VertexBuffer()
{
	m_IndexType = VK_INDEX_TYPE_UINT16;
}


VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::Finalize()
{
	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	m_VertexBuffer = VK_NULL_HANDLE;
	vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
	vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
	m_IndexBuffer = VK_NULL_HANDLE;
	vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
}

void VertexBuffer::CreateVertexBuffer(GraphicSystem* pGraphicSystem, const void* pData, size_t dataSize)
{
	m_Device = pGraphicSystem->GetDevice();
	VkDevice device = pGraphicSystem->GetDevice();
	VkPhysicalDevice physicalDevice = pGraphicSystem->GetPhysicalDevice();

	VkBuffer localBuffer;
	VkDeviceMemory localBufferMemory;
	CreateBuffer(
		&localBuffer,
		&localBufferMemory,
		dataSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		device,
		physicalDevice);

	void* tempData;
	VkResult result = vkMapMemory(device, localBufferMemory, 0, dataSize, 0, &tempData);
	std::memcpy(tempData, pData, dataSize);
	vkUnmapMemory(device, localBufferMemory);

	CreateBuffer(
		&m_VertexBuffer,
		&m_VertexBufferMemory,
		dataSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		device,
		physicalDevice);

	CopyBuffer(localBuffer, m_VertexBuffer, dataSize, device, pGraphicSystem->GetCommandPool(), pGraphicSystem->GetQueues()[0]);

	vkDestroyBuffer(device, localBuffer, nullptr);
	vkFreeMemory(device, localBufferMemory, nullptr);

}

void VertexBuffer::CreateIndexBuffer(GraphicSystem* pGraphicSystem, const void* pData, size_t dataSize, VkIndexType indexType)
{
	m_Device = pGraphicSystem->GetDevice();
	VkDevice device = pGraphicSystem->GetDevice();
	VkPhysicalDevice physicalDevice = pGraphicSystem->GetPhysicalDevice();

	m_IndexType = indexType;

	VkBuffer localBuffer;
	VkDeviceMemory localBufferMemory;
	CreateBuffer(
		&localBuffer,
		&localBufferMemory,
		dataSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		device,
		physicalDevice);

	void* tempData;
	VkResult result = vkMapMemory(device, localBufferMemory, 0, dataSize, 0, &tempData);
	std::memcpy(tempData, pData, dataSize);
	vkUnmapMemory(device, localBufferMemory);

	CreateBuffer(
		&m_IndexBuffer,
		&m_IndexBufferMemory,
		dataSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		device,
		physicalDevice);

	CopyBuffer(localBuffer, m_IndexBuffer, dataSize, device, pGraphicSystem->GetCommandPool(), pGraphicSystem->GetQueues()[0]);

	vkDestroyBuffer(device, localBuffer, nullptr);
	vkFreeMemory(device, localBufferMemory, nullptr);
}