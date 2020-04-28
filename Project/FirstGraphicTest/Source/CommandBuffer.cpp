#include "CommandBuffer.h"



CommandBuffer::CommandBuffer()
{
}


CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::Finalize()
{
	for (VkCommandBuffer commandBuffer : m_CommandBuffers)
	{
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
	}
}

void CommandBuffer::Init(VkDevice device, uint32_t swapCount, VkCommandPool commandPool)
{
	m_Device = device;
	m_CommandPool = commandPool;
	m_CommandBuffers.resize(swapCount);

	VkCommandBufferAllocateInfo allocCmdBufInfo = {};
	allocCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocCmdBufInfo.commandPool = commandPool;
	allocCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocCmdBufInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

	vkAllocateCommandBuffers(m_Device, &allocCmdBufInfo, m_CommandBuffers.data());
}

void CommandBuffer::Begin(uint32_t index)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(m_CommandBuffers[index], &beginInfo);
}
void CommandBuffer::End(uint32_t index)
{
	vkEndCommandBuffer(m_CommandBuffers[index]);
}

VkCommandBuffer CommandBuffer::GetCommandBuffer(uint32_t index)
{
	return m_CommandBuffers[index];
}