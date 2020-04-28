#pragma once
#include <fstream>

#include <vector>

static bool LoadFile(std::vector<char>& fileData, const std::string & filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	fileData.resize(fileSize);

	file.seekg(0);
	file.read(fileData.data(), fileSize);

	file.close();

	return true;
}