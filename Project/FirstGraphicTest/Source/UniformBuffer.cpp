#include "UniformBuffer.h"

UniformBuffer::UniformBuffer()
{
}

UniformBuffer::~UniformBuffer()
{
}

void UniformBuffer::Finalize()
{
	for (uint32_t i = 0; i < m_SwapCount; i++)
	{
		vkDestroyBuffer(m_Device, m_UniformBuffer[i], nullptr);
		m_UniformBuffer[i] = VK_NULL_HANDLE;
		vkFreeMemory(m_Device, m_UniformBufferMemory[i], nullptr);
	}
}

void UniformBuffer::CreateUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, void* pData, size_t dataSize, uint32_t swapCount, int binding, std::vector<VkDescriptorSet>& descriptorSets)
{
	m_Device = device;

	m_SwapCount = swapCount;
	m_UniformBuffer.resize(m_SwapCount);
	m_UniformBufferMemory.resize(m_SwapCount);

	m_BufferInfos.resize(m_SwapCount);
	m_DescriptorWrites.resize(m_SwapCount); 

	m_BufferSize = dataSize;

	for (uint32_t i = 0; i < m_SwapCount; i++)
	{
		CreateBuffer(
			&m_UniformBuffer[i], 
			&m_UniformBufferMemory[i], 
			m_BufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			device,
			physicalDevice);

		if (pData != nullptr)
		{
			void* tempData;
			VkResult result = vkMapMemory(device, m_UniformBufferMemory[i], 0, m_BufferSize, 0, &tempData);
			std::memcpy(tempData, pData, m_BufferSize);
			vkUnmapMemory(device, m_UniformBufferMemory[i]);
		}

		m_BufferInfos[i] = {};
		m_BufferInfos[i].buffer = m_UniformBuffer[i];
		m_BufferInfos[i].offset = 0;
		m_BufferInfos[i].range = m_BufferSize;

		m_DescriptorWrites[i] = {};
		m_DescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		m_DescriptorWrites[i].dstSet = descriptorSets[i];
		m_DescriptorWrites[i].dstBinding = binding;
		m_DescriptorWrites[i].dstArrayElement = 0;
		m_DescriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		m_DescriptorWrites[i].descriptorCount = 1;
		m_DescriptorWrites[i].pBufferInfo = &m_BufferInfos[i];

		vkUpdateDescriptorSets(device, 1, &m_DescriptorWrites[i], 0, nullptr);
	}	
}

void UniformBuffer::UpdateUniformBuffer(VkDevice device, void* pData, size_t dataSize, uint32_t index)
{
	if (dataSize > m_BufferSize || pData == nullptr)
	{
		return;
	}

	void* tempData;
	VkResult result = vkMapMemory(device, m_UniformBufferMemory[index], 0, dataSize, 0, &tempData);
	std::memcpy(tempData, pData, dataSize);
	vkUnmapMemory(device, m_UniformBufferMemory[index]);

}