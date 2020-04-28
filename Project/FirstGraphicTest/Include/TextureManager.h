#pragma once
#include "Helper.h"

struct TextureData
{
	std::string textureName;
	int width;
	int height; 
	int channels;
	int faceCount;
	int mipLevels;
	int bitDepth;
	void* pTexData;
	size_t texDataSize;
	TextureData()
	{
		textureName = "";
		width = 0;
		height = 0;
		channels = 0;
		mipLevels = 1;
		pTexData = nullptr;
		texDataSize = 0;
	}
};

class TextureManager
{
private:
	TextureManager() {};
	~TextureManager() {};
	TextureManager(const TextureManager&);
	TextureManager& operator=(const TextureManager&);

	std::map<std::string, TextureData*> textureDataList;

public:
	void Finalize();
	static TextureManager& GetInstance()
	{
		static TextureManager instance;
		return instance;
	}
	TextureData* GetTextureData(std::string filename)
	{
		if (_access(filename.c_str(), 4) == -1)
		{
			printf("### ERROR ### file[%s] not exist.\n", filename.c_str());
			return nullptr;
		}
		else
		{
			std::map<std::string, TextureData*>::iterator iter;
			iter = textureDataList.find(filename);
			if (iter == textureDataList.end())
			{
				TextureData* pTexData = new TextureData();
				textureDataList.insert(std::map<std::string, TextureData*>::value_type(filename, pTexData));
				//textureDataList[filename] = new TextureData();
			}
			return textureDataList[filename];
		}
	}
	static bool LoadTexture(TextureData** ppTextureData, const std::string & filename);

};

