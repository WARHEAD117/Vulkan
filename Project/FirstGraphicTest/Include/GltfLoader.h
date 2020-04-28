// ------------------------------------------------------------
// Copyright(c) 2019 Jesse Yurkovich
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// See the LICENSE file in the repo root for full license information.
// ------------------------------------------------------------
#pragma once

#include "fxgltf/gltf.h"

class MaterialData
{
public:
	void SetData(fx::gltf::Material const & material)
	{
		m_material = material;
		m_hasData = true;
	}

	fx::gltf::Material const & Data() const noexcept
	{
		return m_material;
	}

	bool HasData() const
	{
		return m_hasData && !m_material.pbrMetallicRoughness.empty();
	}

private:
	fx::gltf::Material m_material{};
	bool m_hasData{};
};

class MeshData
{
public:
	struct BufferInfo
	{
		fx::gltf::Accessor const * Accessor;

		uint8_t const * Data;
		uint32_t DataStride;
		uint32_t TotalSize;

		bool HasData() const noexcept
		{
			return Data != nullptr;
		}
	};

	MeshData(fx::gltf::Document const & doc, std::size_t meshIndex, std::size_t primitveIndex)
	{
		fx::gltf::Mesh const & mesh = doc.meshes[meshIndex];
		fx::gltf::Primitive const & primitive = mesh.primitives[primitveIndex];

		for (auto const & attrib : primitive.attributes)
		{
			if (attrib.first == "POSITION")
			{
				m_vertexBuffer = GetData(doc, doc.accessors[attrib.second]);
			}
			else if (attrib.first == "NORMAL")
			{
				m_normalBuffer = GetData(doc, doc.accessors[attrib.second]);
			}
			else if (attrib.first == "TANGENT")
			{
				m_tangentBuffer = GetData(doc, doc.accessors[attrib.second]);
			}
			else if (attrib.first == "TEXCOORD_0")
			{
				m_texCoord0Buffer = GetData(doc, doc.accessors[attrib.second]);
			}
		}

		m_indexBuffer = GetData(doc, doc.accessors[primitive.indices]);

		if (primitive.material >= 0)
		{
			m_materialData.SetData(doc.materials[primitive.material]);
		}
	}

	BufferInfo const & IndexBuffer() const noexcept
	{
		return m_indexBuffer;
	}

	BufferInfo const & VertexBuffer() const noexcept
	{
		return m_vertexBuffer;
	}

	BufferInfo const & NormalBuffer() const noexcept
	{
		return m_normalBuffer;
	}

	BufferInfo const & TangentBuffer() const noexcept
	{
		return m_tangentBuffer;
	}

	BufferInfo const & TexCoord0Buffer() const noexcept
	{
		return m_texCoord0Buffer;
	}

	MaterialData const & Material() const noexcept
	{
		return m_materialData;
	}

private:
	BufferInfo m_indexBuffer{};
	BufferInfo m_vertexBuffer{};
	BufferInfo m_normalBuffer{};
	BufferInfo m_tangentBuffer{};
	BufferInfo m_texCoord0Buffer{};

	MaterialData m_materialData{};

	static BufferInfo GetData(fx::gltf::Document const & doc, fx::gltf::Accessor const & accessor)
	{
		fx::gltf::BufferView const & bufferView = doc.bufferViews[accessor.bufferView];
		fx::gltf::Buffer const & buffer = doc.buffers[bufferView.buffer];

		const uint32_t dataTypeSize = CalculateDataTypeSize(accessor);
		return BufferInfo{ &accessor, &buffer.data[static_cast<uint64_t>(bufferView.byteOffset) + accessor.byteOffset], dataTypeSize, accessor.count * dataTypeSize };
	}

	static uint32_t CalculateDataTypeSize(fx::gltf::Accessor const & accessor) noexcept
	{
		uint32_t elementSize = 0;
		switch (accessor.componentType)
		{
		case fx::gltf::Accessor::ComponentType::Byte:
		case fx::gltf::Accessor::ComponentType::UnsignedByte:
			elementSize = 1;
			break;
		case fx::gltf::Accessor::ComponentType::Short:
		case fx::gltf::Accessor::ComponentType::UnsignedShort:
			elementSize = 2;
			break;
		case fx::gltf::Accessor::ComponentType::Float:
		case fx::gltf::Accessor::ComponentType::UnsignedInt:
			elementSize = 4;
			break;
		}

		switch (accessor.type)
		{
		case fx::gltf::Accessor::Type::Mat2:
			return 4 * elementSize;
			break;
		case fx::gltf::Accessor::Type::Mat3:
			return 9 * elementSize;
			break;
		case fx::gltf::Accessor::Type::Mat4:
			return 16 * elementSize;
			break;
		case fx::gltf::Accessor::Type::Scalar:
			return elementSize;
			break;
		case fx::gltf::Accessor::Type::Vec2:
			return 2 * elementSize;
			break;
		case fx::gltf::Accessor::Type::Vec3:
			return 3 * elementSize;
			break;
		case fx::gltf::Accessor::Type::Vec4:
			return 4 * elementSize;
			break;
		}

		return 0;
	}
};
#include <string>

class ImageData
{
public:
	struct ImageInfo
	{
		std::string FileName{};

		uint32_t BinarySize{};
		uint8_t const * BinaryData{};

		bool IsBinary() const noexcept
		{
			return BinaryData != nullptr;
		}
	};

	explicit ImageData(std::string const & texture)
	{
		m_info.FileName = texture;
	}

	ImageData(fx::gltf::Document const & doc, std::size_t textureIndex, std::string const & modelPath)
	{
		fx::gltf::Image const & image = doc.images[doc.textures[textureIndex].source];

		const bool isEmbedded = image.IsEmbeddedResource();
		if (!image.uri.empty() && !isEmbedded)
		{
			m_info.FileName = fx::gltf::detail::GetDocumentRootPath(modelPath) + "/" + image.uri;
		}
		else
		{
			if (isEmbedded)
			{
				image.MaterializeData(m_embeddedData);
				m_info.BinaryData = &m_embeddedData[0];
				m_info.BinarySize = static_cast<uint32_t>(m_embeddedData.size());
			}
			else
			{
				fx::gltf::BufferView const & bufferView = doc.bufferViews[image.bufferView];
				fx::gltf::Buffer const & buffer = doc.buffers[bufferView.buffer];

				m_info.BinaryData = &buffer.data[bufferView.byteOffset];
				m_info.BinarySize = bufferView.byteLength;
			}
		}
	}

	ImageInfo const & Info() const noexcept
	{
		return m_info;
	}

private:
	ImageInfo m_info{};

	std::vector<uint8_t> m_embeddedData{};
};