#include "RenderObject.h"

#include "Light.h"

RenderObject::RenderObject()
{
	m_ModelMtx = glm::mat4(1);
	m_WorldMtx = glm::mat4(1);

	m_IsDoubleSided = false;
}


RenderObject::~RenderObject()
{
}

void RenderObject::SetGraphicSystem(GraphicSystem* pGraphicSystem)
{
	m_pGraphicSystem = pGraphicSystem;
	m_Device = pGraphicSystem->GetDevice();
	m_PhysicalDevice = pGraphicSystem->GetPhysicalDevice();
}

void RenderObject::Finalize()
{
	m_UniformBufferDescriptor.Finalize();
	m_LightInfosDescriptor.Finalize();
	m_VertexBuffer.Finalize();
	for (TextureDescriptor* pTexDescriptor : m_TextureDescriptors)
	{
		if (pTexDescriptor->isInitialized)
		{
			pTexDescriptor->texture.Finalize();
		}
	}

	vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
}

void RenderObject::Init()
{
	uint32_t swapChainCount = m_pGraphicSystem->GetSwapChainCount();
	VkExtent2D extent = m_pGraphicSystem->GetSwapChainExtent();
	VkRenderPass renderPass = m_pGraphicSystem->GetRenderPass();

	m_UniformBufferDescriptor.Init(m_pGraphicSystem, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	m_LightInfosDescriptor.Init(m_pGraphicSystem, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	std::vector<VkDescriptorSetLayoutBinding> bindings = 
	{ 
		m_UniformBufferDescriptor.uniformBinding,
		m_LightInfosDescriptor.uniformBinding
	};

	for (TextureDescriptor* pTexDescriptor : m_TextureDescriptors)
	{
		bindings.push_back(pTexDescriptor->textureBinding);
	}

	CreateDescriptorSetLayout(&m_DescriptorSetLayout, bindings, m_Device);

	std::vector<VkDescriptorSetLayout> layouts(swapChainCount, m_DescriptorSetLayout);

	VkDescriptorPoolSize uniformDescriptorPoolSize = {};
	VkDescriptorPoolSize samplerDescriptorPoolSize = {};
	CreateDescriptorPoolSize(&uniformDescriptorPoolSize, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, swapChainCount);
	CreateDescriptorPoolSize(&samplerDescriptorPoolSize, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, swapChainCount);

	std::vector<VkDescriptorPoolSize> poolSizes = { uniformDescriptorPoolSize , samplerDescriptorPoolSize };

	m_DescriptorPool = {};
	CreateDescriptorPool(&m_DescriptorPool, m_Device, poolSizes, swapChainCount);

	m_DescriptorSets.resize(swapChainCount);
	m_DescriptorSetAlocateInfo = {};
	m_DescriptorSetAlocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	m_DescriptorSetAlocateInfo.descriptorPool = m_DescriptorPool;
	m_DescriptorSetAlocateInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	m_DescriptorSetAlocateInfo.pSetLayouts = layouts.data();

	VkResult result = vkAllocateDescriptorSets(m_Device, &m_DescriptorSetAlocateInfo, m_DescriptorSets.data());

	m_UniformBufferDescriptor.uniformBuffer.CreateUniformBuffer(m_Device, m_PhysicalDevice, nullptr, sizeof(UniformData), swapChainCount, 0, m_DescriptorSets);

	m_LightInfosDescriptor.uniformBuffer.CreateUniformBuffer(m_Device, m_PhysicalDevice, nullptr, sizeof(LightInfosUniform), swapChainCount, 1, m_DescriptorSets);

	//-----------------------------------------------------------------------

	CreatePipelineLayout(&m_PipelineLayout, m_Device, &m_DescriptorSetLayout);

	//-----------------------------------------------------------------------

	VkPipelineShaderStageCreateInfo shaderStages[2]; 

	specializationMapEntry.constantID = 0;
	specializationMapEntry.offset = 0;
	specializationMapEntry.size = sizeof(uint32_t);

	specializationMapEntry2.constantID = 1;
	specializationMapEntry2.offset = sizeof(uint32_t);
	specializationMapEntry2.size = sizeof(uint32_t);
	VkSpecializationMapEntry mapEntries[] = { specializationMapEntry ,specializationMapEntry2 };

	uint32_t pData[] = { m_VertexAttributeFlags ,m_TextureAttributeFlags };

	specializationInfo.mapEntryCount = 2;
	specializationInfo.pMapEntries = mapEntries;
	specializationInfo.dataSize = sizeof(uint32_t) + sizeof(uint32_t);
	specializationInfo.pData = pData;

	CreateShaderStage(&shaderStages[0], m_VertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT, &specializationInfo);
	CreateShaderStage(&shaderStages[1], m_FragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	auto bindingDescription = Vertex::GetBindingDescription(0);
	auto attributeDescriptions = Vertex::GetAttributeDescription(0);
	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	CreateVertexInputState(&vertexInputState, &bindingDescription, &attributeDescriptions);

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	CreateInputAssemblyState(&inputAssemblyState);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = extent;
	VkPipelineViewportStateCreateInfo viewportState = {};
	CreateViewportState(&viewportState, &viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	CreateRasterizationState(&rasterizationState);
	if (m_IsDoubleSided)
	{
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
	}
	
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	CreateMultisampleState(&multisampleState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	CreateDepthStencilState(&depthStencilState);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	CreateColorBlendAttachmentState(&colorBlendAttachment);

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	CreateColorBlendState(&colorBlendState, &colorBlendAttachment);

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	CreateDynamicState(&dynamicState);
	VkGraphicsPipelineCreateInfo pipelinCreateInfo = {};
	pipelinCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelinCreateInfo.stageCount = 2;
	pipelinCreateInfo.pStages = shaderStages;
	pipelinCreateInfo.pVertexInputState = &vertexInputState;
	pipelinCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelinCreateInfo.pViewportState = &viewportState;
	pipelinCreateInfo.pRasterizationState = &rasterizationState;
	pipelinCreateInfo.pMultisampleState = &multisampleState;
	pipelinCreateInfo.pDepthStencilState = &depthStencilState;
	pipelinCreateInfo.pTessellationState = nullptr;
	pipelinCreateInfo.pColorBlendState = &colorBlendState;
	pipelinCreateInfo.pDynamicState = nullptr;// &dynamicState;

	pipelinCreateInfo.layout = m_PipelineLayout;
	pipelinCreateInfo.renderPass = renderPass;
	pipelinCreateInfo.subpass = 0;

	pipelinCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelinCreateInfo.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelinCreateInfo, nullptr, &m_Pipeline);

	//--------------------------------------------------------

	for (TextureDescriptor* pTexDescriptor : m_TextureDescriptors)
	{
		pTexDescriptor->Bind(m_DescriptorSets);
	}

}
void RenderObject::SetGeometry(const void* pVertexData, size_t vertexCount, const void* pIndexData, size_t indexCount, int indexStride, uint32_t vertexAttributeFlag)
{
	m_VertexCount = vertexCount;
	m_IndexCount = indexCount;

	VkIndexType indexType = VK_INDEX_TYPE_UINT16;
	switch (indexStride)
	{
	case 1:
		indexType = VK_INDEX_TYPE_UINT8_EXT;
		break;
	case 2:
		indexType = VK_INDEX_TYPE_UINT16;
		break;
	case 4:
		indexType = VK_INDEX_TYPE_UINT32;
		break;
	default:
		break;
	}

	m_VertexBuffer.CreateVertexBuffer(m_pGraphicSystem, pVertexData, m_VertexCount * sizeof(Vertex));
	m_VertexBuffer.CreateIndexBuffer(m_pGraphicSystem, pIndexData, m_IndexCount * indexStride, indexType);
	m_VertexAttributeFlags = vertexAttributeFlag;
}

void RenderObject::SetDiffuseTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_DiffuseTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_SRGB, pTexData, texDataSize, TextureBindingOffset + 0);
	m_TextureDescriptors.push_back(&m_DiffuseTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetDiffuseTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	m_DiffuseTextureDescriptor.Init(m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_SRGB,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 0);
	m_TextureDescriptors.push_back(&m_DiffuseTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetNormalTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_NormalTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_UNORM, pTexData, texDataSize, TextureBindingOffset + 1);
	m_TextureDescriptors.push_back(&m_NormalTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetNormalTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	m_NormalTextureDescriptor.Init(m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_UNORM,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 1);
	m_TextureDescriptors.push_back(&m_NormalTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetMetallicRoughnessTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_MetallicRoughnessTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_UNORM, pTexData, texDataSize, TextureBindingOffset + 2);
	m_TextureDescriptors.push_back(&m_MetallicRoughnessTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetMetallicRoughnessTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	m_MetallicRoughnessTextureDescriptor.Init(
		m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_UNORM,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 2);
	m_TextureDescriptors.push_back(&m_MetallicRoughnessTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetEmissiveTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_EmissiveTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_UNORM, pTexData, texDataSize, TextureBindingOffset + 3);
	m_TextureDescriptors.push_back(&m_EmissiveTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetEmissiveTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	m_EmissiveTextureDescriptor.Init(
		m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_UNORM,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 3);
	m_TextureDescriptors.push_back(&m_EmissiveTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetOcclusionTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_OcclusionTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_UNORM, pTexData, texDataSize, TextureBindingOffset + 4);
	m_TextureDescriptors.push_back(&m_OcclusionTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetOcclusionTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	if (textureData == nullptr && textureAttributeFlags == OCCLUSION_IN_METALLICROUGHNESS_TEX)
	{
		//m_TextureAttributeFlags |= textureAttributeFlags;
		//return;
	}

	m_OcclusionTextureDescriptor.Init(
		m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_UNORM,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 4);
	m_TextureDescriptors.push_back(&m_OcclusionTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetDfgTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_DfgTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_UNORM, pTexData, texDataSize, TextureBindingOffset + 7);
	m_TextureDescriptors.push_back(&m_DfgTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetDfgTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	m_DfgTextureDescriptor.Init(m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_UNORM,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 7);
	m_TextureDescriptors.push_back(&m_DfgTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetIBLTexture(int width, int height, int mipLevels, void* pTexData, size_t texDataSize, uint32_t textureAttributeFlags)
{
	m_IBLTextureDescriptor.Init(m_pGraphicSystem, width, height, 1, mipLevels, 32, VK_FORMAT_R8G8B8A8_UNORM, pTexData, texDataSize, TextureBindingOffset + 8);
	m_TextureDescriptors.push_back(&m_IBLTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}

void RenderObject::SetIBLTexture(TextureData* textureData, uint32_t textureAttributeFlags)
{
	m_IBLTextureDescriptor.Init(m_pGraphicSystem,
		textureData->width,
		textureData->height,
		textureData->faceCount,
		textureData->mipLevels,
		textureData->bitDepth,
		VK_FORMAT_R8G8B8A8_UNORM,
		textureData->pTexData,
		textureData->texDataSize,
		TextureBindingOffset + 8);
	m_TextureDescriptors.push_back(&m_IBLTextureDescriptor);
	m_TextureAttributeFlags |= textureAttributeFlags;
}


void RenderObject::SetVertexShaderModule(VkShaderModule shaderModule)
{
	m_VertexShaderModule = shaderModule;
}
void RenderObject::SetFragmentShaderModule(VkShaderModule shaderModule)
{
	m_FragmentShaderModule = shaderModule;
}

void RenderObject::Update(uint32_t index)
{
	m_UniformData.modelMtx = m_WorldMtx * m_ModelMtx;
	m_UniformData.viewMtx = m_pGraphicSystem->GetCamera().viewMtx;
	m_UniformData.projMtx = m_pGraphicSystem->GetCamera().projMtx;

	m_UniformData.cameraPos = glm::vec4(m_pGraphicSystem->GetCamera().cameraPos, 1.0);

	m_UniformBufferDescriptor.uniformBuffer.UpdateUniformBuffer(m_Device, &m_UniformData, sizeof(UniformData), index);


	m_LightInfosData.lightCount.x = static_cast<float>(LightManager::GetInstance().GetLightCount());
	for (int i = 0; i < m_LightInfosData.lightCount.x; i++)
	{
		Light* pLight = LightManager::GetInstance().GetLight(i);
		LightInfo& info = m_LightInfosData.lightInfos[i];
		info.lightColor = pLight->GetLightColorIntensity();
		info.LightDir = glm::vec4(pLight->GetLightDir(), 1.0f);
		info.LightPos = glm::vec4(pLight->GetLightPos(), 1.0f);
		info.lightInfo.x = pLight->GetLightRange();
		info.lightInfo.y = pLight->GetInnerConeAngleCos();
		info.lightInfo.z = pLight->GetOuterConeAngleCos();
		info.lightInfo.w = pLight->GetLightType();
	}
	m_LightInfosDescriptor.uniformBuffer.UpdateUniformBuffer(m_Device, &m_LightInfosData, sizeof(LightInfosUniform), index);
}
void RenderObject::Draw(VkCommandBuffer commandBuffer, uint32_t index)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[index], 0, nullptr);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, m_VertexBuffer.GetVertexBuffer(), offsets);
	vkCmdBindIndexBuffer(commandBuffer, *m_VertexBuffer.GetIndexBuffer(), 0, m_VertexBuffer.GetIndexType());
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_IndexCount), 1, 0, 0, 0);
}