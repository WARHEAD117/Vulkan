#pragma once
#include "Helper.h"

class CommandBuffer
{
public:
	CommandBuffer();
	~CommandBuffer();

	void Finalize();

	void Init(VkDevice device, uint32_t swapCount, VkCommandPool commandPool);

	void Begin(uint32_t index);
	void End(uint32_t index);

	VkCommandBuffer GetCommandBuffer(uint32_t index);

private:
	VkDevice m_Device;
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};

