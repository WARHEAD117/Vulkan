#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstring>
#include <array>

#include <iostream>
#include <vector>
#include <map>
#include <optional>
#include <set>

#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#include <cstdint>
#include <algorithm>

#include <chrono>

static glm::quat MakeQuat(glm::vec3 srcVec, glm::vec3 dstVec)
{
	glm::vec3 axis = glm::cross(srcVec, dstVec);
	float angle = glm::angle(glm::normalize(srcVec), glm::normalize(dstVec));
	glm::quat quat;
	quat = glm::rotate(quat, angle, axis);
	return quat;
}
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 uv;

	static VkVertexInputBindingDescription GetBindingDescription(uint32_t binding)
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = binding;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescription(uint32_t binding)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescription(4);

		attributeDescription[0].binding = binding;
		attributeDescription[0].location = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].offset = offsetof(Vertex, pos);

		attributeDescription[1].binding = binding;
		attributeDescription[1].location = 1;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].offset = offsetof(Vertex, normal);

		attributeDescription[2].binding = binding;
		attributeDescription[2].location = 2;
		attributeDescription[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription[2].offset = offsetof(Vertex, tangent);

		attributeDescription[3].binding = binding;
		attributeDescription[3].location = 3;
		attributeDescription[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[3].offset = offsetof(Vertex, uv);
		return attributeDescription;
	}
};

static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	// TODO
	return 0;
}

static void CreateDescriptorPoolSize(VkDescriptorPoolSize* pDescriptorPoolSize, VkDescriptorType type, uint32_t swapCount)
{
	*pDescriptorPoolSize = {};
	pDescriptorPoolSize->descriptorCount = swapCount;
	pDescriptorPoolSize->type = type;
}

static bool CreateDescriptorPool(VkDescriptorPool* pDescriptorPool, VkDevice device, std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t swapCount)
{
	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = 1024;// swapCount;

	VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, pDescriptorPool);
	if (result != VK_SUCCESS)
	{
		return false;
	}
	return true;
}

static void CreateDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding* pLayoutBinding, uint32_t bindingPoint, VkDescriptorType type, VkShaderStageFlags flag, VkDevice device)
{
	*pLayoutBinding = {};
	pLayoutBinding->binding = bindingPoint;
	pLayoutBinding->descriptorType = type;
	pLayoutBinding->descriptorCount = 1;
	pLayoutBinding->stageFlags = flag;
	pLayoutBinding->pImmutableSamplers = nullptr;
}

static bool CreateDescriptorSetLayout(VkDescriptorSetLayout* pDescriptorSetLayout, std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDevice device)
{
	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, pDescriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

static bool CreateDescriptorSet(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorSetAllocateInfo* pDescriptorSetAlocateInfo, VkDescriptorPool descriptorPool, std::vector<VkDescriptorSetLayout>& layouts, VkDevice device)
{
	pDescriptorSetAlocateInfo->sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	pDescriptorSetAlocateInfo->descriptorPool = descriptorPool;
	pDescriptorSetAlocateInfo->descriptorSetCount = static_cast<uint32_t>(layouts.size());
	pDescriptorSetAlocateInfo->pSetLayouts = layouts.data();

	VkResult result = vkAllocateDescriptorSets(device, pDescriptorSetAlocateInfo, descriptorSets.data());
	if (result != VK_SUCCESS)
	{
		return false;
	}

	return true;
}


static bool CreatePipelineLayout(VkPipelineLayout* pPipelineLayout, VkDevice device, VkDescriptorSetLayout* pDescriptorSetLayout)
{
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = pDescriptorSetLayout;
	createInfo.pushConstantRangeCount = 0;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(device, &createInfo, nullptr, pPipelineLayout);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

static void CreateShaderStage(VkPipelineShaderStageCreateInfo* pCreateInfo, VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, VkSpecializationInfo* pSpecializationInfo)
{
	*pCreateInfo = {};
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pCreateInfo->stage = shaderStage;
	pCreateInfo->module = shaderModule;
	pCreateInfo->pName = "main";
	pCreateInfo->pSpecializationInfo = pSpecializationInfo;
}

static void CreateVertexInputState(
	VkPipelineVertexInputStateCreateInfo* pCreateInfo,
	VkVertexInputBindingDescription* pBindingDescription,
	std::vector<VkVertexInputAttributeDescription>* pAttributeDescriptions)
{
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pCreateInfo->vertexBindingDescriptionCount = 1;
	pCreateInfo->vertexAttributeDescriptionCount = static_cast<uint32_t>(pAttributeDescriptions->size());
	pCreateInfo->pVertexBindingDescriptions = pBindingDescription;
	pCreateInfo->pVertexAttributeDescriptions = pAttributeDescriptions->data();
}

static void CreateInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo* pCreateInfo)
{
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pCreateInfo->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pCreateInfo->primitiveRestartEnable = VK_FALSE;
}

static void CreateViewportState(VkPipelineViewportStateCreateInfo* pCreateInfo, VkViewport* pViewport, VkRect2D* pScissor)
{
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pCreateInfo->viewportCount = 1;
	pCreateInfo->pViewports = pViewport;
	pCreateInfo->scissorCount = 1;
	pCreateInfo->pScissors = pScissor;
}

static void CreateRasterizationState(VkPipelineRasterizationStateCreateInfo* pCreateinfo)
{
	pCreateinfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pCreateinfo->depthClampEnable = VK_FALSE;
	pCreateinfo->rasterizerDiscardEnable = VK_FALSE;
	pCreateinfo->polygonMode = VK_POLYGON_MODE_FILL;
	pCreateinfo->cullMode = VK_CULL_MODE_BACK_BIT;
	pCreateinfo->frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pCreateinfo->depthBiasEnable = VK_FALSE;
	pCreateinfo->depthBiasConstantFactor = 0.0f;
	pCreateinfo->depthBiasClamp = 0.0f;
	pCreateinfo->depthBiasSlopeFactor = 0.0f;
	pCreateinfo->lineWidth = 1.0f;
}

static void CreateMultisampleState(VkPipelineMultisampleStateCreateInfo* pCreateinfo)
{
	pCreateinfo->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pCreateinfo->sampleShadingEnable = VK_FALSE;
	pCreateinfo->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pCreateinfo->minSampleShading = 1.0f;
	pCreateinfo->pSampleMask = nullptr;
	pCreateinfo->alphaToCoverageEnable = VK_FALSE;
	pCreateinfo->alphaToOneEnable = VK_FALSE;
}

static void CreateDepthStencilState(VkPipelineDepthStencilStateCreateInfo* pCreateInfo)
{
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pCreateInfo->depthTestEnable = VK_TRUE;
	pCreateInfo->depthWriteEnable = VK_TRUE;
	pCreateInfo->depthCompareOp = VK_COMPARE_OP_LESS;
	pCreateInfo->depthBoundsTestEnable = VK_FALSE;
	pCreateInfo->minDepthBounds = 0.0f;
	pCreateInfo->maxDepthBounds = 1.0f;
	pCreateInfo->stencilTestEnable = VK_FALSE;
	pCreateInfo->front = {};
	pCreateInfo->back = {};
}

static void CreateColorBlendAttachmentState(VkPipelineColorBlendAttachmentState* pColorBlendAttachment)
{
	pColorBlendAttachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	pColorBlendAttachment->blendEnable = VK_TRUE;
	pColorBlendAttachment->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	pColorBlendAttachment->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pColorBlendAttachment->colorBlendOp = VK_BLEND_OP_ADD;
	pColorBlendAttachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	pColorBlendAttachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pColorBlendAttachment->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void CreateColorBlendState(VkPipelineColorBlendStateCreateInfo* pCreateInfo, VkPipelineColorBlendAttachmentState* pColorBlendAttachment)
{
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pCreateInfo->logicOpEnable = VK_FALSE;
	pCreateInfo->logicOp = VK_LOGIC_OP_COPY;
	pCreateInfo->attachmentCount = 1;
	pCreateInfo->pAttachments = pColorBlendAttachment;
	pCreateInfo->blendConstants[0] = 0.0f;
	pCreateInfo->blendConstants[1] = 0.0f;
	pCreateInfo->blendConstants[2] = 0.0f;
	pCreateInfo->blendConstants[3] = 0.0f;
}

static const VkDynamicState g_DynamicStates[] =
{
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_LINE_WIDTH
};

static void CreateDynamicState(VkPipelineDynamicStateCreateInfo* pCreateInfo)
{
	pCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pCreateInfo->dynamicStateCount = 2;
	pCreateInfo->pDynamicStates = g_DynamicStates;
}

static bool CreateShaderModule(VkShaderModule* pVkShaderModule, VkDevice device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, pVkShaderModule);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	return true;
}


static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

static void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRagion = {};
	copyRagion.srcOffset = 0;
	copyRagion.dstOffset = 0;
	copyRagion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRagion);

	EndSingleTimeCommands(commandBuffer, device, commandPool, queue);
}

static void CreateBuffer(
	VkBuffer* pBuffer,
	VkDeviceMemory* pBufferMemory,
	VkDeviceSize size,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags propertyFlags,
	VkDevice device,
	VkPhysicalDevice physicalDevice)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, pBuffer);

	VkMemoryRequirements memRequirments;
	vkGetBufferMemoryRequirements(device, *pBuffer, &memRequirments);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirments.size;
	allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirments.memoryTypeBits, propertyFlags);

	result = vkAllocateMemory(device, &allocInfo, nullptr, pBufferMemory);

	result = vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0);
}

static void CreateImage(
	VkImage* pImage, 
	VkDeviceMemory* pImageMemory, 
	VkDevice device, 
	VkPhysicalDevice physicalDevice, 
	uint32_t width,
	uint32_t height, 
	uint32_t layers,
	uint32_t mipLevels,
	VkImageType imageType,
	VkFormat format, 
	VkImageTiling tiling,
	VkImageUsageFlags usageFlags,
	VkMemoryPropertyFlags propertyFlags)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = imageType;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = layers;

	imageInfo.format = format;
	imageInfo.tiling = tiling;//VK_IMAGE_TILING_LINEAR //VK_IMAGE_TILING_OPTIMAL
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usageFlags;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;
	if (layers == 6)
	{
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	vkCreateImage(device, &imageInfo, nullptr, pImage);

	VkMemoryRequirements memRequirments;
	vkGetImageMemoryRequirements(device, *pImage, &memRequirments);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirments.size;
	allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirments.memoryTypeBits, propertyFlags);

	VkResult result = vkAllocateMemory(device, &allocInfo, nullptr, pImageMemory);

	result = vkBindImageMemory(device, *pImage, *pImageMemory, 0);
}

static bool CreateImageView(VkImageView* pImageView, VkImage image, VkImageViewType viewType, VkDevice device, uint32_t layers, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;

	VkResult result = vkCreateImageView(device, &viewInfo, nullptr, pImageView);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

static void TransitionImageLayout(VkImage image, VkFormat format, uint32_t layers, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);


	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layers;

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer, device, commandPool, queue);
}

static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, uint32_t bitDepth, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

	uint32_t offset = 0;
	std::vector<VkBufferImageCopy> bufferCopyRegions = {};
	for (uint32_t face = 0; face < layers; face++) {
		for (uint32_t level = 0; level < mipLevels; level++) {
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width >> level;
			bufferCopyRegion.imageExtent.height = height >> level;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegion.imageOffset = { 0,0,0 };

			bufferCopyRegions.push_back(bufferCopyRegion);

			
			// Increase offset into staging buffer for next level / face
			offset += (width >> level) * (height >> level) * (bitDepth / 8);
		}
	}

	vkCmdCopyBufferToImage(
		commandBuffer, 
		buffer, 
		image, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		static_cast<uint32_t>(bufferCopyRegions.size()), 
		bufferCopyRegions.data());
	
	EndSingleTimeCommands(commandBuffer, device, commandPool, queue);
}