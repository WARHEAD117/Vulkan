#pragma once
#include "Helper.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"
#include "Texture.h"

#include "GraphicSystem.h"
#include "TextureManager.h"

struct UniformData
{
	glm::mat4 modelMtx;
	glm::mat4 viewMtx;
	glm::mat4 projMtx;
	glm::vec4 cameraPos;
	glm::vec4 baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec4 metallicRoughness = glm::vec4(1.0f,1.0f,0.0f,0.0f);// y : roughness, z : metallic
	glm::vec4 blendMode = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f); // x : blendMode, y : alphaCutOff, 
	glm::vec4 emissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
};

struct LightInfo
{
	glm::vec4 LightPos; // xyz: lightPos
	glm::vec4 LightDir; // xyz: lightDir
	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // xyz: color, w : intensity
	glm::vec4 lightInfo = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);// x: range, y: innerAngleCos, z: outerAngleCos w: lightType
};

struct LightInfosUniform
{
	LightInfo lightInfos[16];
	glm::vec4 lightCount = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
};

class RenderObject
{
public:
	RenderObject();
	~RenderObject();
	void Finalize();

	void SetGraphicSystem(GraphicSystem* pGraphicSystem);

	void SetGeometry(const void* pVertexData, size_t vertexCount, const void* pIndexData, size_t indexCount, int indexStride, uint32_t vertexAttributeFlags);
	
	void SetDiffuseTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetDiffuseTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetNormalTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetNormalTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetMetallicRoughnessTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetMetallicRoughnessTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetEmissiveTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetEmissiveTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetOcclusionTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetOcclusionTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetDfgTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetDfgTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetIBLTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags);
	void SetIBLTexture(TextureData* textureData, uint32_t textureAttributeFlags);

	void SetVertexShaderModule(VkShaderModule shaderModule);
	void SetFragmentShaderModule(VkShaderModule shaderModule);

	void Init();

	void Update(uint32_t index);
	VkBuffer* GetVertexBuffer()
	{
		return m_VertexBuffer.GetVertexBuffer();
	}
	VkBuffer* GetIndexBuffer()
	{
		return m_VertexBuffer.GetIndexBuffer();
	}
	UniformBuffer* GetUniformBuffer()
	{
		return &m_UniformBuffer;
	}

	void Draw(VkCommandBuffer commandBuffer, uint32_t index);


	std::vector<VkDescriptorSet>& GetDescriptorSets()
	{
		return m_DescriptorSets;
	}

	void SetModelTransform(glm::mat4 transform)
	{
		m_ModelMtx = transform;
	}

	void SetWorldTransform(glm::mat4 transform)
	{
		m_WorldMtx = transform;
	}

	void SetMaterial(glm::vec4 baseColorFactor, float metallicFactor, float roughnessFactor, glm::vec3 emissiveFactor, float blendMode, float alphaMaskValue, bool isDoubleSided)
	{
		m_UniformData.baseColorFactor = baseColorFactor;
		m_UniformData.metallicRoughness.x = metallicFactor;
		m_UniformData.metallicRoughness.y = roughnessFactor;
		m_UniformData.blendMode.x = blendMode;
		m_UniformData.blendMode.y = alphaMaskValue;
		m_UniformData.emissiveFactor = glm::make_vec4(emissiveFactor);
		m_UniformData.emissiveFactor.w = 0.0f;

		m_IsDoubleSided = isDoubleSided;
	}

private:

	GraphicSystem* m_pGraphicSystem;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;

	VertexBuffer m_VertexBuffer;

	UniformBuffer m_UniformBuffer;
	//UniformBuffer m_LightInfo;
	//UniformBuffer m_MaterialInfo;
	UniformBuffer m_LightInfoBuffer;


	struct TextureDescriptor
	{
		bool isInitialized = false;
		Texture texture; 
		uint32_t bindingPoint;
		VkDescriptorSetLayoutBinding textureBinding;
		void Init(
			GraphicSystem* pGraphicSystem,
			int width,
			int height,
			int layers,
			int mipLevels,
			int bitDepth,
			VkFormat format,
			void* pTexData,
			size_t texDataSize, 
			uint32_t binding)
		{
			isInitialized = true;

			VkFormat texFormat = format;
			if (bitDepth == 64)
			{
				texFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			}
			else if (bitDepth == 128)
			{
				texFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			VkImageType imageType = VK_IMAGE_TYPE_2D;
			VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
			if (layers == 6)
			{
				imageType = VK_IMAGE_TYPE_2D;
				imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
			}
			texture.Init(pGraphicSystem, width, height, layers, mipLevels, imageType, imageViewType, texFormat, pTexData, texDataSize);
			bindingPoint = binding;
			CreateDescriptorSetLayoutBinding(&textureBinding, bindingPoint, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, pGraphicSystem->GetDevice());
		}
		void Bind(std::vector<VkDescriptorSet>& descriptorSets)
		{
			texture.Bind(descriptorSets, bindingPoint);
		}
	};
	TextureDescriptor m_DiffuseTextureDescriptor;
	TextureDescriptor m_NormalTextureDescriptor;
	TextureDescriptor m_MetallicRoughnessTextureDescriptor;
	TextureDescriptor m_EmissiveTextureDescriptor;
	TextureDescriptor m_OcclusionTextureDescriptor;
	TextureDescriptor m_DfgTextureDescriptor;
	TextureDescriptor m_IBLTextureDescriptor;
	std::vector<TextureDescriptor*> m_TextureDescriptors;

	struct UniformDescriptor
	{
		bool isInitialized = false;
		UniformBuffer uniformBuffer;
		uint32_t bindingPoint;
		VkDescriptorSetLayoutBinding uniformBinding;
		void Init(
			GraphicSystem* pGraphicSystem, uint32_t binding, VkShaderStageFlags shaderStage)
		{
			isInitialized = true;
			bindingPoint = binding;//VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
			CreateDescriptorSetLayoutBinding(&uniformBinding, bindingPoint, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStage, pGraphicSystem->GetDevice());
		}
		void Bind(std::vector<VkDescriptorSet>& descriptorSets)
		{
			//texture.Bind(descriptorSets, bindingPoint);
		}
		void Finalize()
		{
			if (isInitialized == true)
			{
				uniformBuffer.Finalize();
			}
		}
	};
	UniformDescriptor m_UniformBufferDescriptor;
	UniformDescriptor m_LightInfosDescriptor;

	VkSpecializationInfo specializationInfo;
	VkSpecializationMapEntry specializationMapEntry;
	VkSpecializationMapEntry specializationMapEntry2;
	uint32_t m_VertexAttributeFlags;
	uint32_t m_TextureAttributeFlags;

	VkShaderModule m_VertexShaderModule;
	VkShaderModule m_FragmentShaderModule;

	VkDescriptorSetLayout m_DescriptorSetLayout;

	std::vector<VkDescriptorSet> m_DescriptorSets; 
	VkDescriptorSetAllocateInfo m_DescriptorSetAlocateInfo;

	VkDescriptorPool m_DescriptorPool;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;

	UniformData m_UniformData;
	glm::mat4 m_WorldMtx;
	glm::mat4 m_ModelMtx;

	BOOL m_IsDoubleSided;

	LightInfosUniform m_LightInfosData;

	size_t m_VertexCount;
	size_t m_IndexCount;

	static const uint32_t TextureBindingOffset = 2;
};

