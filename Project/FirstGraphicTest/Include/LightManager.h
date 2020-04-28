#pragma once
#include "Helper.h"

class Light;

class LightManager
{
private:
	LightManager() {};
	~LightManager() {}
	LightManager(const LightManager&);
	LightManager& operator=(const LightManager&);

	std::vector<Light*> lightList;

public:
	void Finalize();
	static LightManager& GetInstance()
	{
		static LightManager instance;
		return instance;
	}
	Light* CreateNewLight();

	void AddLight(Light* pLight);

	Light* GetLight(int index);

	int GetLightCount();
};

