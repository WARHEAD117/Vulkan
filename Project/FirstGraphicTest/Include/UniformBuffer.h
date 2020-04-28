#pragma once
#include "Helper.h"

class UniformBuffer
{
public:
	UniformBuffer();
	~UniformBuffer();

	void Finalize();

	void CreateUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, void* pData, size_t dataSize, uint32_t swapCount, int binding, std::vector<VkDescriptorSet>& descriptorSets);


	void UpdateUniformBuffer(VkDevice device, void* pData, size_t dataSize, uint32_t index);

	VkBuffer* GetUniformBuffer(uint32_t index)
	{
		return &m_UniformBuffer[index];
	}

	VkWriteDescriptorSet& GetDescriptorWrite(uint32_t index)
	{
		return m_DescriptorWrites[index];
	}

private:
	std::vector<VkBuffer> m_UniformBuffer;
	std::vector < VkDeviceMemory> m_UniformBufferMemory;
	size_t m_BufferSize;
	uint32_t m_SwapCount;

	std::vector < VkDescriptorBufferInfo> m_BufferInfos;
	std::vector<VkWriteDescriptorSet> m_DescriptorWrites;

	VkDevice m_Device;
};

