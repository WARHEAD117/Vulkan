#include "LightManager.h"
#include "Light.h"

void LightManager::Finalize()
{
	for (Light* pLight : lightList)
	{
		delete(pLight);
		pLight = nullptr;
	}
	lightList.clear();
}

Light* LightManager::CreateNewLight()
{
	Light* pLight = new Light();
	lightList.push_back(pLight);
	return pLight;
}

void LightManager::AddLight(Light* pLight)
{
	lightList.push_back(pLight);
}

Light* LightManager::GetLight(int index)
{
	if (static_cast<size_t>(index) >= lightList.size())
	{
		return nullptr;
	}
	return lightList[index];
}

int LightManager::GetLightCount()
{
	int lightCount = static_cast<int>(lightList.size());

	return lightCount;
}