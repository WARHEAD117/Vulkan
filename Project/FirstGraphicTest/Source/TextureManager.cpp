#include "TextureManager.h"

#include "SOIL2.h"

void TextureManager::Finalize()
{
	for (auto data : textureDataList)
	{
		free(data.second->pTexData); 
		data.second->pTexData = nullptr;
		delete(data.second);
		data.second = nullptr;
	}
	textureDataList.clear();
}

bool TextureManager::LoadTexture(TextureData** ppTextureData, const std::string & filename)
{
	TextureData* pTextureData = TextureManager::GetInstance().GetTextureData(filename);
	if (pTextureData == nullptr)
	{
		ppTextureData = nullptr;
		return false;
	}

	if (pTextureData->pTexData == nullptr)
	{
		pTextureData->textureName = filename;
		
		//stbi_uc* pixels = stbi_load(filename.c_str(), &pTextureData->width, &pTextureData->height, &pTextureData->channels, STBI_rgb_alpha);
		uint8_t* pixels = SOIL_load_image_full(filename.c_str(), &pTextureData->width, &pTextureData->height, &pTextureData->channels, &pTextureData->faceCount, &pTextureData->mipLevels, &pTextureData->bitDepth, SOIL_LOAD_RGBA);
		
		for (int faceIndex = 0; faceIndex < pTextureData->faceCount; faceIndex++)
		{
			for (int mipIndex = 0; mipIndex < pTextureData->mipLevels; mipIndex++)
			{
				int curW = pTextureData->width >> mipIndex;
				int curH = pTextureData->height >> mipIndex;
				
				pTextureData->texDataSize += curW * curH * (pTextureData->bitDepth / 8);
			}
		}

		pTextureData->pTexData = malloc(pTextureData->texDataSize);

		memcpy(pTextureData->pTexData, pixels, pTextureData->texDataSize);

		SOIL_free_image_data(pixels);
		pixels = nullptr;
	}

	*ppTextureData = pTextureData;

	return true;
}
