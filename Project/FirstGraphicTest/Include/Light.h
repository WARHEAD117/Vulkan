#pragma once
#include "Helper.h"
#include "LightManager.h"

enum LightType
{
	DIRECTIONAL_LIGHT = 0,
	POINT_LIGHT = 1,
	SPOT_LIGHT = 2
};

class Light
{
public:
	Light()
	{
		m_LightIntensity = 1;
		m_LightLocalMtx = glm::mat4(1);
		m_LightWorldMtx = glm::mat4(1);
	}

	~Light()
	{
	}

	void SetLightType(LightType type)
	{
		m_LightType = type;
	}

	uint8_t GetLightType()
	{
		return m_LightType;
	}

	glm::vec3 GetLightPos()
	{
		m_LightPos = glm::vec3(m_LightMtx[3]);
		return m_LightPos;
	}

	glm::vec3 GetLightDir()
	{
		m_LightDir = -glm::vec3(m_LightMtx[2]);
		return glm::normalize(m_LightDir);
	}

	void SetLightColor(glm::vec3 color)
	{
		m_LightColor = color;
	}

	void SetLightIntensity(float intensity)
	{
		m_LightIntensity = intensity;
	}

	glm::vec3 GetLightColor()
	{
		return m_LightColor;
	}

	glm::vec4 GetLightColorIntensity()
	{
		return glm::vec4(m_LightColor, m_LightIntensity);
	}

	void SetLightColor(float r, float g, float b)
	{
		m_LightColor = glm::vec3(r,g,b);
	}

	void SetLightRange(float range)
	{
		m_LightRange = range;
	}

	float GetLightRange()
	{
		return m_LightRange;
	}

	void SetInnerConeAngle(float angle)
	{
		m_InnerConeAngle = angle;
	}

	float GetInnerConeAngle()
	{
		return m_InnerConeAngle;
	}

	float GetInnerConeAngleCos()
	{
		return glm::cos(m_InnerConeAngle);
	}

	void SetOuterConeAngle(float angle)
	{
		m_OuterConeAngle = angle;
	}

	float GetOuterConeAngle()
	{
		return m_OuterConeAngle;
	}

	float GetOuterConeAngleCos()
	{
		return glm::cos(m_OuterConeAngle);
	}

	glm::mat4  GetLightLocalTransform()
	{
		return m_LightLocalMtx;
	}

	void SetLightLocalTransform(glm::mat4 transform)
	{
		m_LightLocalMtx = transform;
		m_LightMtx = m_LightWorldMtx * m_LightLocalMtx;
	}

	void SetLightWorldTransform(glm::mat4 transform)
	{
		m_LightWorldMtx = transform;
		m_LightMtx = m_LightWorldMtx * m_LightLocalMtx;
	}

	LightType m_LightType;
	glm::vec3 m_LightPos;
	glm::vec3 m_LightDir;
	glm::vec3 m_LightColor;
	float m_LightIntensity;
	float m_LightRange;
	float m_InnerConeAngle;
	float m_OuterConeAngle;
	glm::mat4 m_LightLocalMtx;
	glm::mat4 m_LightWorldMtx;
	glm::mat4 m_LightMtx;
};

