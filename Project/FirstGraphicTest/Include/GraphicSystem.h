#pragma once
#include "Helper.h"

struct Camera
{
	glm::vec3 cameraPos;
	glm::vec3 cameraLookAt;
	glm::vec3 cameraUp;
	glm::mat4 viewMtx;
	glm::mat4 projMtx;
};

class GraphicSystem
{
public:
	GraphicSystem();
	~GraphicSystem();
	void Finalize();
	void InitGraphicsSystem(GLFWwindow* pWindow);

	VkDevice GetDevice()
	{
		return m_Device;
	}
	VkPhysicalDevice GetPhysicalDevice()
	{
		return m_PhysicalDevice;
	}
	VkFormat GetSwapChainFormat()
	{
		return m_SwapChainFormat;
	}
	VkInstance GetInstance()
	{
		return m_Instance;
	}
	std::vector<VkQueue>& GetQueues()
	{
		return m_Queues;
	}

	VkSwapchainKHR GetSwapChain()
	{
		return m_SwapChain;
	}
	VkExtent2D GetSwapChainExtent()
	{
		return m_SwapChainExtent;
	}
	VkSurfaceKHR GetSurface()
	{
		return m_Surface;
	}

	uint32_t GetSwapChainCount()
	{
		return m_SwapChainCount;
	}
	VkRenderPass GetRenderPass()
	{
		return m_RenderPass;
	}
	VkCommandPool GetCommandPool()
	{
		return m_CommandPool;
	}

	std::vector<VkFramebuffer>& GetSwapChainFrameBuffers()
	{
		return m_SwapChainFrameBuffers;
	}

	VkImageView GetDepthImageView()
	{
		return m_DepthImageView;
	}

	float GetSwapChainAspect()
	{
		return m_SwapChainExtent.width / (1.0f * m_SwapChainExtent.height);
	}

	Camera& GetCamera()
	{
		return m_Camera;
	}

private:
	VkFormat m_SwapChainFormat;
	VkInstance m_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	std::vector<VkQueue> m_Queues;
	VkSwapchainKHR m_SwapChain;
	VkExtent2D m_SwapChainExtent;
	VkSurfaceKHR m_Surface;
	VkRenderPass m_RenderPass;
	VkCommandPool m_CommandPool;

	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	std::vector<VkFramebuffer> m_SwapChainFrameBuffers;

	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	int m_ScreenWidth;
	int m_ScreenHeight;
	uint32_t m_SwapChainCount;

	Camera m_Camera;
};

