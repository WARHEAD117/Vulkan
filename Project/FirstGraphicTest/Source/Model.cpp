#include "Model.h"

#include "FileReader.h"
#include "GltfLoader.h"
#include "TextureManager.h"
#include "Light.h"

#include <thread>

namespace
{
	TextureData* pNullTextureData;
	TextureData* pDfgTextureData;
	TextureData* pIBLTextureData;
	void CreateObject(Mesh& mesh, const fx::gltf::Document& model, int meshIndex, GraphicSystem* pGraphicSystem, std::vector<TextureData*>& textureDatas)
	{
		for (std::size_t i = 0; i < model.meshes[meshIndex].primitives.size(); i++)
		{
			MeshData meshData(model, meshIndex, i);
			MeshData::BufferInfo const & vBuffer = meshData.VertexBuffer();
			MeshData::BufferInfo const & nBuffer = meshData.NormalBuffer();
			MeshData::BufferInfo const & tBuffer = meshData.TangentBuffer();
			MeshData::BufferInfo const & cBuffer = meshData.TexCoord0Buffer();
			MeshData::BufferInfo const & iBuffer = meshData.IndexBuffer();
			MaterialData const & material = meshData.Material();
			if (!vBuffer.HasData() || !nBuffer.HasData() || !iBuffer.HasData())
			{
				throw std::runtime_error("Only meshes with vertex, normal, and index buffers are supported");
			}
			RenderObject* pObject = new RenderObject();
			pObject->SetGraphicSystem(pGraphicSystem);
			//pObject->Init(pGraphicSystem);
			std::vector<Vertex> vertexs;

			uint32_t vertexAttributeFlags = 0;
			const float* vData = reinterpret_cast<const float*>(vBuffer.Data);
			const float* nData = reinterpret_cast<const float*>(nBuffer.Data);
			const float* tData = reinterpret_cast<const float*>(tBuffer.Data);
			const float* cData = reinterpret_cast<const float*>(cBuffer.Data);
			for (uint32_t vIndex = 0; vIndex < vBuffer.TotalSize / vBuffer.DataStride; vIndex++)
			{
				float x = 0;
				float y = 0;
				float z = 0;
				if (vData != nullptr)
				{
					vertexAttributeFlags |= POSION;
					int componentStride = vBuffer.DataStride / 4;
					x = vData[vIndex * 3];
					y = vData[vIndex * 3 + 1];
					z = vData[vIndex * 3 + 2];
				}

				float nx = 0;
				float ny = 0;
				float nz = 0;
				if (nData != nullptr)
				{
					vertexAttributeFlags |= NORMAL;
					int componentStride = nBuffer.DataStride / 4;
					nx = nData[vIndex * componentStride];
					ny = nData[vIndex * componentStride + 1];
					nz = nData[vIndex * componentStride + 2];
				}

				float tx = 0;
				float ty = 0;
				float tz = 0;
				float tw = 1.0;
				if (tData != nullptr)
				{
					vertexAttributeFlags |= TANGENT;
					int componentStride = tBuffer.DataStride / 4;
					tx = tData[vIndex * componentStride];
					ty = tData[vIndex * componentStride + 1];
					tz = tData[vIndex * componentStride + 2];
					tw = tData[vIndex * componentStride + 3];
				}

				float u = 0;
				float v = 0;
				if (cData != nullptr)
				{
					vertexAttributeFlags |= TEXCOORD;
					int componentStride = cBuffer.DataStride / 4;
					u = cData[vIndex * componentStride];
					v = cData[vIndex * componentStride + 1];
				}

				Vertex vtx =
				{
					{x,y,z},
					{nx,ny,nz},
					{tx,ty,tz,tw},
					{u,v},
				};
				vertexs.push_back(vtx);
			}
			if (iBuffer.DataStride == 1)
			{
				std::vector<uint16_t> indexBuffer(iBuffer.TotalSize / iBuffer.DataStride);
				for (uint32_t i = 0; i < iBuffer.TotalSize / iBuffer.DataStride; i++)
				{
					indexBuffer[i] = iBuffer.Data[i];
				}

				pObject->SetGeometry(vertexs.data(), vertexs.size(), indexBuffer.data(), iBuffer.TotalSize / iBuffer.DataStride, sizeof(uint16_t), vertexAttributeFlags);
			}
			else
			{
				pObject->SetGeometry(vertexs.data(), vertexs.size(), reinterpret_cast<const void*>(iBuffer.Data), iBuffer.TotalSize / iBuffer.DataStride, iBuffer.DataStride, vertexAttributeFlags);
			}

			int diffuseTexIndex = material.Data().pbrMetallicRoughness.baseColorTexture.index;
			int normalTexIndex = material.Data().normalTexture.index;
			int metallicRoughnessTexIndex = material.Data().pbrMetallicRoughness.metallicRoughnessTexture.index;
			int emissiveTexIndex = material.Data().emissiveTexture.index;
			int occlusionTexIndex = material.Data().occlusionTexture.index;
			if (diffuseTexIndex != -1)
			{
				pObject->SetDiffuseTexture(textureDatas[diffuseTexIndex], DIFFUSE_TEX);
			}
			else
			{
				pObject->SetDiffuseTexture(pNullTextureData, 0);
			}
			if (normalTexIndex != -1)
			{
				pObject->SetNormalTexture(textureDatas[normalTexIndex], NORMAL_TEX);
			}
			else
			{
				pObject->SetNormalTexture(pNullTextureData, 0);
			}
			if (metallicRoughnessTexIndex != -1)
			{
				pObject->SetMetallicRoughnessTexture(textureDatas[metallicRoughnessTexIndex], METALLICROUGHNESS_TEX);
			}
			else
			{
				pObject->SetMetallicRoughnessTexture(pNullTextureData, 0);
			}
			if (emissiveTexIndex != -1)
			{
				pObject->SetEmissiveTexture(textureDatas[emissiveTexIndex], EMISSIVE_TEX);
			}
			else
			{
				pObject->SetEmissiveTexture(pNullTextureData, 0);
			}
			if (occlusionTexIndex != -1)
			{
				if (occlusionTexIndex == metallicRoughnessTexIndex)
				{
					pObject->SetOcclusionTexture(pNullTextureData, OCCLUSION_IN_METALLICROUGHNESS_TEX);
				}
				else
				{
					pObject->SetOcclusionTexture(textureDatas[occlusionTexIndex], OCCLUSION_TEX);
				}
			}
			else
			{
				pObject->SetOcclusionTexture(pNullTextureData, 0);
			}
			//PBR-Dfg
			pObject->SetDfgTexture(pDfgTextureData, DFG_TEX);
			//PBR-IBL
			pObject->SetIBLTexture(pIBLTextureData, IBL_TEX);

			if (material.HasData())
			{
				float metallicFactor = material.Data().pbrMetallicRoughness.metallicFactor;
				float roughnessFactor = material.Data().pbrMetallicRoughness.roughnessFactor;
				glm::vec4 baseColorFactor = glm::make_vec4(material.Data().pbrMetallicRoughness.baseColorFactor.data());
				glm::vec4 emissiveFactor = glm::make_vec4(material.Data().emissiveFactor.data());
				pObject->SetMaterial(baseColorFactor, metallicFactor, roughnessFactor, emissiveFactor, (float)material.Data().alphaMode, material.Data().alphaCutoff, material.Data().doubleSided);
			}
			mesh.push_back(pObject);
		}
	}
	struct Node
	{
		glm::mat4 currentTransform;
		int32_t meshIndex = -1;
		int32_t lightIndex = -1;
	};
	void Visit(fx::gltf::Document const & doc, uint32_t nodeIndex, glm::mat4 const & parentTransform, std::vector<Node> & graphNodes)
	{
		Node & graphNode = graphNodes[nodeIndex];
		graphNode.currentTransform = parentTransform;

		fx::gltf::Node const & node = doc.nodes[nodeIndex];
		if (node.matrix != fx::gltf::defaults::IdentityMatrix)
		{
			const glm::mat4 local = glm::make_mat4(node.matrix.data());
			graphNode.currentTransform = local * graphNode.currentTransform;
		}
		else
		{
			if (node.translation != fx::gltf::defaults::NullVec3)
			{
				const glm::vec3 local = glm::make_vec3(node.translation.data());
				graphNode.currentTransform = glm::translate(graphNode.currentTransform, local);// DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&local)) *;
			}

			if (node.scale != fx::gltf::defaults::IdentityVec3)
			{
				const glm::vec3 local = glm::make_vec3(node.scale.data());
				graphNode.currentTransform = glm::scale(graphNode.currentTransform, local);// DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&local)) * graphNode.currentTransform;
			}

			if (node.rotation != fx::gltf::defaults::IdentityVec4)
			{
				const glm::quat local = glm::make_quat(node.rotation.data());
				graphNode.currentTransform = graphNode.currentTransform * glm::mat4_cast(local);
			}
		}

		if (node.mesh >= 0)
		{
			graphNode.meshIndex = node.mesh;
		}

		if (node.light >= 0)
		{
			graphNode.lightIndex = node.light;
		}

		for (auto childIndex : node.children)
		{
			Visit(doc, childIndex, graphNode.currentTransform, graphNodes);
		}
	}

	void LoadTextureJob(std::vector<TextureData*>& textureDatas, int startIndex, int count, fx::gltf::Document& model, std::string textureRoot)
	{
		for (int i = startIndex; i < startIndex + count; i++)
		{
			ImageData image(model, i, "\\");
			std::string textureName = textureRoot + image.Info().FileName;

			TextureManager::GetInstance().LoadTexture(&textureDatas[i], textureName);
		}
	}


}

Model::Model()
{
	m_Translate = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);
	m_Rotate = glm::quat();
	m_WorldTransform = glm::mat4(1.0);

}


Model::~Model()
{
}

void Model::Finalize()
{
	vkDestroyShaderModule(m_Device, m_VsShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, m_FsShaderModule, nullptr);

	for (Mesh mesh : meshes)
	{
		for (RenderObject* pObj : mesh)
		{
			pObj->Finalize();
			delete(pObj);
			pObj = nullptr;
		}
	}
}
void Model::CreateModel(std::string textureName, const void* pVertexData, size_t vertexCount, const void* pIndexData, size_t indexCount, GraphicSystem* pGraphicSystem)
{
	m_Device = pGraphicSystem->GetDevice();
	std::vector<char> vsCode;
	std::vector<char> fsCode;
	LoadFile(vsCode, "Shader/vs.spv");
	LoadFile(fsCode, "Shader/fs.spv");

	CreateShaderModule(&m_VsShaderModule, m_Device, vsCode);
	CreateShaderModule(&m_FsShaderModule, m_Device, fsCode);

	{
		TextureManager::GetInstance().LoadTexture(&pNullTextureData, "Texture/white.png");
		TextureManager::GetInstance().LoadTexture(&pDfgTextureData, "Texture/IBLTestBrdf.dds");
		TextureManager::GetInstance().LoadTexture(&pIBLTextureData, "Texture/IBLTestSpecularHDR.dds");
	}

	meshes.resize(1);
	RenderObject* pObj = new RenderObject();
	meshes[0].push_back(pObj);
	pObj->SetGraphicSystem(pGraphicSystem);
	pObj->SetGeometry(
		pVertexData, vertexCount,
		pIndexData, indexCount, sizeof(uint16_t),
		1);
	TextureData* pTexData;
	TextureManager::GetInstance().LoadTexture(&pTexData, textureName);
	pObj->SetDiffuseTexture(pTexData, DIFFUSE_TEX);
	pObj->SetNormalTexture(pNullTextureData, 0);
	pObj->SetMetallicRoughnessTexture(pNullTextureData, 0);
	pObj->SetEmissiveTexture(pNullTextureData, 0);
	pObj->SetOcclusionTexture(pNullTextureData, 0);
	pObj->SetDfgTexture(pNullTextureData, 0);
	pObj->SetIBLTexture(pIBLTextureData, 0);

	pObj->SetVertexShaderModule(m_VsShaderModule);
	pObj->SetFragmentShaderModule(m_FsShaderModule);
	pObj->Init();
}
void Model::CreateModel(std::string fileName, std::string textureRoot, GraphicSystem* pGraphicSystem)
{
	m_Device = pGraphicSystem->GetDevice();
	std::vector<char> vsCode;
	std::vector<char> fsCode;
	LoadFile(vsCode, "Shader/vs.spv");
	LoadFile(fsCode, "Shader/fs.spv");

	CreateShaderModule(&m_VsShaderModule, m_Device, vsCode);
	CreateShaderModule(&m_FsShaderModule, m_Device, fsCode);

	{
		TextureManager::GetInstance().LoadTexture(&pNullTextureData, "Texture/white.png");
		TextureManager::GetInstance().LoadTexture(&pDfgTextureData, "Texture/IBLTestBrdf.dds");
		TextureManager::GetInstance().LoadTexture(&pIBLTextureData, "Texture/IBLTestSpecularHDR.dds");
	}

	fx::gltf::Document model = fx::gltf::LoadFromText(fileName.c_str()); //NormalTangentTest//Sponza//Sponza2
	std::vector<TextureData*> textureDatas(model.textures.size());

	lights.resize(model.lights.size());
	for (size_t i = 0; i < lights.size(); i++)
	{
		fx::gltf::Light& modelLight = model.lights[i];
		lights[i] = LightManager::GetInstance().CreateNewLight();
		LightType lightType = LightType::POINT_LIGHT;
		switch (modelLight.type)
		{
		case fx::gltf::Light::Type::Directional:
			lightType = LightType::DIRECTIONAL_LIGHT;
			break;
		case fx::gltf::Light::Type::Point:
			lightType = LightType::POINT_LIGHT;
			break;
		case fx::gltf::Light::Type::Spot:
			lightType = LightType::SPOT_LIGHT;
			break;
		default:
			lightType = LightType::POINT_LIGHT;
			break;
		}
		lights[i]->SetLightType(lightType);
		lights[i]->SetLightColor(glm::make_vec3(modelLight.color.data()));
		lights[i]->SetLightRange(modelLight.range);
		lights[i]->SetLightIntensity(modelLight.intensity);
		lights[i]->SetInnerConeAngle(modelLight.spot.innerConeAngle);
		lights[i]->SetOuterConeAngle(modelLight.spot.outerConeAngle);

	}

	meshes.resize(model.meshes.size());

	{
		static const int JobCount = 8;
		std::thread* pThreadObjects[JobCount];

		int textureCount = static_cast<int>(model.textures.size());
		for (int i = 0; i < JobCount; i++)
		{
			int taskCount = (textureCount + JobCount) / JobCount;
			int startIndex = i * taskCount;
			int count = taskCount;
			if (startIndex + taskCount > textureCount)
			{
				count -= (startIndex + taskCount - textureCount);
			}
			pThreadObjects[i] = new std::thread(LoadTextureJob, std::ref(textureDatas), startIndex, count, std::ref(model), textureRoot);
		}
		for (int i = 0; i < JobCount; i++)
		{
			pThreadObjects[i]->join();
		}

		for (uint32_t i = 0; i < model.meshes.size(); i++)
		{
			CreateObject(meshes[i], model, i, pGraphicSystem, textureDatas);
		}
		std::vector<Node> graphNodes(model.nodes.size());
		for (const uint32_t sceneNode : model.scenes[0].nodes)
		{
			Visit(model, sceneNode, glm::mat4(1.0f), graphNodes);
		}
		for (auto & graphNode : graphNodes)
		{
			if (graphNode.meshIndex >= 0)
			{
				for (RenderObject* pObj : meshes[graphNode.meshIndex])
				{
					pObj->SetModelTransform(graphNode.currentTransform);
				}
			}
			if (graphNode.lightIndex >= 0)
			{
				lights[graphNode.lightIndex]->SetLightLocalTransform(graphNode.currentTransform);
			}
		}
		for (Mesh& mesh : meshes)
		{
			for (RenderObject* pObj : mesh)
			{
				pObj->SetVertexShaderModule(m_VsShaderModule);
				pObj->SetFragmentShaderModule(m_FsShaderModule);
				pObj->Init();
			}
		}

		for (int i = 0; i < JobCount; i++)
		{
			delete(pThreadObjects[i]);
			pThreadObjects[i] = nullptr;
		}
	}

}
void Model::Draw(VkCommandBuffer commandBuffer, uint32_t index)
{
	for (Mesh mesh : meshes)
	{
		for (RenderObject* pObj : mesh)
		{
			pObj->Draw(commandBuffer, index);
		}
	}
}

void Model::Update(uint32_t index)
{
	m_WorldTransform = glm::translate(glm::mat4(1.0f), m_Translate);// DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&local)) *;
	m_WorldTransform = glm::scale(m_WorldTransform, m_Scale);// DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&local)) * graphNode.currentTransform;
	m_WorldTransform = glm::mat4_cast(m_Rotate) * m_WorldTransform;
	
	for (Light* pLight : lights)
	{
		pLight->SetLightWorldTransform(m_WorldTransform);
	}

	for (Mesh mesh : meshes)
	{
		for (RenderObject* pObj : mesh)
		{
			pObj->SetWorldTransform(m_WorldTransform);
			pObj->Update(index);
		}
	}
}