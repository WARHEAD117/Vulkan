#include "GraphicSystem.h"

namespace
{
	const int WIDTH = 1920;
	const int HEIGHT = 1080;

#ifdef NDEBUG
	static const bool EnableValidationLayers = false;
#else
	static const bool EnableValidationLayers = true;
#endif

	static const std::vector<const char*> ValidationLayers = {
	   "VK_LAYER_KHRONOS_validation"
	};

	static const std::vector<const char*> DeviceExtensions = {
	   "VK_KHR_swapchain",
	   "VK_EXT_index_type_uint8"
	};

	bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
	{

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool isLayerAvailable = false;
			for (const VkLayerProperties& prop : availableLayers)
			{
				if (strcmp(layerName, prop.layerName) == 0)
				{
					isLayerAvailable = true;
					break;
				}
			}
			if (!isLayerAvailable)
			{
				return false;
			}
		}
		return true;
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

		for (const VkExtensionProperties& ext : availableExtensions)
		{
			requiredExtensions.erase(ext.extensionName);
		}

		return requiredExtensions.empty();
	}

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool hasValue()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties& prop : queueFamilies)
		{
			if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			{
				VkBool32 presentSupport = false;

				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

				if (presentSupport)
				{
					indices.presentFamily = i;
				}
			}

			if (indices.hasValue())
			{
				break;
			}

			i++;
		}
		return indices;
	}

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
		}
		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = { WIDTH, HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	bool InitVulkan(
		VkInstance* pVkInstance,
		VkPhysicalDevice* pVkPhysicalDevice,
		VkDevice* pVkDevice,
		std::vector<VkQueue>& m_Queues,
		VkSwapchainKHR* pVkSwapChain,
		VkFormat* swapChainFormat,
		VkExtent2D* m_SwapChainExtent,
		VkSurfaceKHR* pVkSurface,
		GLFWwindow* pWindow)
	{
		VkResult result = VK_SUCCESS;

		auto CreateVkInstance = [](VkInstance* pVkInstance)
		{
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = "First Graphic Test";
			appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
			appInfo.pEngineName = "No Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_2;

			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;

			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			createInfo.enabledExtensionCount = glfwExtensionCount;
			createInfo.ppEnabledExtensionNames = glfwExtensions;


			if (CheckValidationLayerSupport(ValidationLayers) && EnableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
				createInfo.ppEnabledLayerNames = ValidationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			VkResult restult = vkCreateInstance(&createInfo, nullptr, pVkInstance);
			return restult;
		};
		result = CreateVkInstance(pVkInstance);


		{
			VkWin32SurfaceCreateInfoKHR surfaceCreateinfo = {};
			surfaceCreateinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surfaceCreateinfo.hwnd = glfwGetWin32Window(pWindow);
			surfaceCreateinfo.hinstance = GetModuleHandle(nullptr);

			result = vkCreateWin32SurfaceKHR(*pVkInstance, &surfaceCreateinfo, nullptr, pVkSurface);

			if (result != VK_SUCCESS)
			{
				return false;
			}

			result = glfwCreateWindowSurface(*pVkInstance, pWindow, nullptr, pVkSurface);

			if (result != VK_SUCCESS)
			{
				return false;
			}
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::cout << extensionCount << " extensions supported" << std::endl;

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "available extensions:\n" << std::endl;

		for (const VkExtensionProperties& ext : extensions)
		{
			std::cout << ext.extensionName << std::endl;
		}


		if (result != VK_SUCCESS)
		{
			return false;
		}

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(*pVkInstance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(*pVkInstance, &deviceCount, physicalDevices.data());
		*pVkPhysicalDevice = physicalDevices.data()[0];
		const VkPhysicalDevice physicalDevice = *pVkPhysicalDevice;

		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

		const VkSurfaceKHR surface = *pVkSurface;
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

		bool isDeviceExtensionSupport = CheckDeviceExtensionSupport(physicalDevice);
		bool swapChainAdequate = false;

		SwapChainSupportDetails swapChainSupportDetails = QuerySwapChainSupport(physicalDevice, surface);

		if (isDeviceExtensionSupport)
		{
			swapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
		}

		if (!indices.hasValue() || !CheckDeviceExtensionSupport(physicalDevice) || !swapChainAdequate || !physicalDeviceFeatures.samplerAnisotropy)
		{
			return false;
		}


		{
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			float queuePriority = 1.0f;

			for (uint32_t queueFamily : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}


			VkPhysicalDeviceFeatures physicalDeviceFeature = {};

			VkDeviceCreateInfo deviceCreateInfo = {};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeature;

			deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
			deviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

			if (EnableValidationLayers)
			{
				deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
				deviceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
			}
			else
			{
				deviceCreateInfo.enabledLayerCount = 0;
			}

			result = vkCreateDevice(*pVkPhysicalDevice, &deviceCreateInfo, nullptr, pVkDevice);

			if (result != VK_SUCCESS)
			{
				return false;
			}
		}

		{
			VkQueue graphicsQueue;
			VkQueue presentQueue;
			vkGetDeviceQueue(*pVkDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
			vkGetDeviceQueue(*pVkDevice, indices.presentFamily.value(), 0, &presentQueue);
			m_Queues.push_back(graphicsQueue);
			m_Queues.push_back(presentQueue);
		}

		// CreateSwapChain
		{
			VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
			VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);
			VkExtent2D extent = ChooseSwapExtent(swapChainSupportDetails.capabilities);

			uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
			if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount)
			{
				imageCount = swapChainSupportDetails.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
			swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCreateInfo.surface = surface;
			swapchainCreateInfo.minImageCount = imageCount;
			swapchainCreateInfo.imageFormat = surfaceFormat.format;
			swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
			swapchainCreateInfo.imageExtent = extent;
			swapchainCreateInfo.imageArrayLayers = 1;
			swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),indices.presentFamily.value() };
			if (indices.graphicsFamily != indices.presentFamily)
			{
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapchainCreateInfo.queueFamilyIndexCount = 2;
				swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				swapchainCreateInfo.queueFamilyIndexCount = 0;
				swapchainCreateInfo.pQueueFamilyIndices = nullptr;
			}
			swapchainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
			swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainCreateInfo.presentMode = presentMode;
			swapchainCreateInfo.clipped = VK_TRUE;
			swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

			result = vkCreateSwapchainKHR(*pVkDevice, &swapchainCreateInfo, nullptr, pVkSwapChain);

			*swapChainFormat = surfaceFormat.format;
			*m_SwapChainExtent = extent;

			if (result != VK_SUCCESS)
			{
				return false;
			}
		}
		return true;
	}

	bool CreateRenderPass(VkRenderPass* pRenderPass, VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = colorFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &dependency;

		VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, pRenderPass);
		if (result != VK_SUCCESS)
		{
			return false;
		}

		return true;
	}


}

GraphicSystem::GraphicSystem()
{
}


GraphicSystem::~GraphicSystem()
{
}
void GraphicSystem::Finalize()
{
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
	vkDestroyImage(m_Device, m_DepthImage, nullptr);
	vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

	for (VkImageView swapChainView : m_SwapChainImageViews)
	{
		vkDestroyImageView(m_Device, swapChainView, nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	vkDestroyDevice(m_Device, nullptr);
}

void GraphicSystem::InitGraphicsSystem(GLFWwindow* pWindow)
{
	glfwGetFramebufferSize(pWindow, &m_ScreenWidth, &m_ScreenHeight);

	InitVulkan(&m_Instance, &m_PhysicalDevice, &m_Device, m_Queues, &m_SwapChain, &m_SwapChainFormat, &m_SwapChainExtent, &m_Surface, pWindow);

	// SwapChain
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_SwapChainCount, nullptr);
	m_SwapChainImages.resize(m_SwapChainCount);
	m_SwapChainImageViews.resize(m_SwapChainCount);
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &m_SwapChainCount, m_SwapChainImages.data());
	for (uint32_t i = 0; i < m_SwapChainCount; i++)
	{
		CreateImageView(&m_SwapChainImageViews[i], m_SwapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, m_Device, 1, 1, m_SwapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	//Depth
	CreateImage(
		&m_DepthImage, 
		&m_DepthImageMemory, 
		m_Device, 
		m_PhysicalDevice, 
		m_SwapChainExtent.width, 
		m_SwapChainExtent.height,
		1,
		1,
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_D32_SFLOAT, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CreateImageView(&m_DepthImageView, m_DepthImage, VK_IMAGE_VIEW_TYPE_2D, m_Device, 1, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);

	CreateRenderPass(&m_RenderPass, m_Device, m_SwapChainFormat, VK_FORMAT_D32_SFLOAT);

	m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());
	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
	{
		std::vector<VkImageView> attachments = { m_SwapChainImageViews[i], m_DepthImageView };
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_RenderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = m_SwapChainExtent.width;
		createInfo.height = m_SwapChainExtent.height;
		createInfo.layers = 1;
		vkCreateFramebuffer(m_Device, &createInfo, nullptr, &m_SwapChainFrameBuffers[i]);
	}
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_Surface);
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	cmdPoolCreateInfo.flags = 0;


	vkCreateCommandPool(m_Device, &cmdPoolCreateInfo, nullptr, &m_CommandPool);
}
