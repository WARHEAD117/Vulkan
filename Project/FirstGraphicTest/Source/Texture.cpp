#include "Texture.h"



Texture::Texture()
{
}


Texture::~Texture()
{
}

void Texture::Finalize()
{
	vkDestroySampler(m_Device, m_Sampler, nullptr);
	vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
	vkDestroyImage(m_Device, m_TextureImage, nullptr);
	vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);
}

void Texture::Init(
	GraphicSystem* pGraphicSystem,
	int width, 
	int height,
	int layers,
	int mipLevels,
	VkImageType imageType,
	VkImageViewType viewType,
	VkFormat format,
	void* pTexData, 
	size_t texDataSize)
{
	m_Device = pGraphicSystem->GetDevice();
	VkDevice device = pGraphicSystem->GetDevice();
	VkPhysicalDevice physicalDevice = pGraphicSystem->GetPhysicalDevice();

	VkBuffer localBuffer;
	VkDeviceMemory localBufferMemory;
	CreateBuffer(
		&localBuffer,
		&localBufferMemory,
		texDataSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		device,
		physicalDevice);

	void* tempData;
	VkResult result = vkMapMemory(device, localBufferMemory, 0, texDataSize, 0, &tempData);
	std::memcpy(tempData, pTexData, texDataSize);
	vkUnmapMemory(device, localBufferMemory);

	// Image
	CreateImage(
		&m_TextureImage, 
		&m_TextureImageMemory, 
		device, physicalDevice, 
		width, height, 
		layers,
		mipLevels,
		imageType,
		format,
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	TransitionImageLayout(
		m_TextureImage, 
		format,
		layers,
		mipLevels,
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		device, pGraphicSystem->GetCommandPool(), pGraphicSystem->GetQueues()[0]);

	uint32_t bitDepth = 32;
	if (format == VK_FORMAT_R8G8B8A8_UNORM)
	{
		bitDepth = 32;
	}
	else if (format == VK_FORMAT_R16G16B16A16_UNORM || format == VK_FORMAT_R16G16B16A16_SFLOAT)
	{
		bitDepth = 64;
	}
	else if (format == VK_FORMAT_R32G32B32A32_SFLOAT)
	{
		bitDepth = 128;
	}
	CopyBufferToImage(localBuffer, m_TextureImage, width, height, layers, mipLevels, bitDepth, device, pGraphicSystem->GetCommandPool(), pGraphicSystem->GetQueues()[0]);
	
	TransitionImageLayout(
		m_TextureImage,
		format,
		layers,
		mipLevels,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		device, pGraphicSystem->GetCommandPool(), pGraphicSystem->GetQueues()[0]);


	// ImageView
	CreateImageView(&m_TextureImageView, m_TextureImage, viewType, device, layers, mipLevels, format, VK_IMAGE_ASPECT_COLOR_BIT);

	// Sampler
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR; //VK_FILTER_LINEAR
	samplerInfo.minFilter = VK_FILTER_LINEAR; //VK_FILTER_NEAREST
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.minLod = 0;

	vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);


	vkDestroyBuffer(device, localBuffer, nullptr);
	vkFreeMemory(device, localBufferMemory, nullptr);
}

void Texture::Bind(std::vector<VkDescriptorSet>& descriptorSets, uint32_t binding)
{
	m_ImageInfos.resize(descriptorSets.size());
	m_DescriptorWrites.resize(descriptorSets.size());
	for (size_t i = 0; i < descriptorSets.size(); i++)
	{
		m_ImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_ImageInfos[i].imageView = m_TextureImageView;
		m_ImageInfos[i].sampler = m_Sampler;

		m_DescriptorWrites[i] = {};
		m_DescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		m_DescriptorWrites[i].dstSet = descriptorSets[i];
		m_DescriptorWrites[i].dstBinding = binding;
		m_DescriptorWrites[i].dstArrayElement = 0;
		m_DescriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		m_DescriptorWrites[i].descriptorCount = 1;
		m_DescriptorWrites[i].pImageInfo = &m_ImageInfos[i];

		vkUpdateDescriptorSets(m_Device, 1, &m_DescriptorWrites[i], 0, nullptr);
	}
}