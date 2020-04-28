#pragma once
#include "Helper.h"
#include "GraphicSystem.h"

enum TextureAttributeFlag
{
	DIFFUSE_TEX = 1,
	NORMAL_TEX = 1 << 1,
	METALLICROUGHNESS_TEX = 1 << 2,
	EMISSIVE_TEX = 1 << 3,
	OCCLUSION_TEX = 1 << 4,
	OCCLUSION_IN_METALLICROUGHNESS_TEX = 1 << 5,
	DFG_TEX = 1 << 8,
	IBL_TEX = 1 << 9,
};

class Texture
{
public:
	Texture();
	~Texture();

	void Finalize();

	void Init(
		GraphicSystem* pGraphicSystem,
		int width,
		int height,
		int layers,
		int mipLevels,
		VkImageType imageType,
		VkImageViewType viewType,
		VkFormat format,
		void* pTexData,
		size_t texDataSize);

	void Bind(std::vector<VkDescriptorSet>& descriptorSets, uint32_t binding);

	VkWriteDescriptorSet& GetWriteDescriptorSets(uint32_t index)
	{
		return m_DescriptorWrites[index];
	}

private:
	VkDevice m_Device;

	VkImage m_TextureImage;
	VkImageView m_TextureImageView;
	VkDeviceMemory m_TextureImageMemory;
	VkSampler m_Sampler;

	std::vector<VkDescriptorImageInfo> m_ImageInfos;
	std::vector<VkWriteDescriptorSet> m_DescriptorWrites;
};

