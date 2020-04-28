#pragma once
#include "Helper.h"

#include "GraphicSystem.h"

#include "RenderObject.h"
#include "CommandBuffer.h"



#include "GltfLoader.h"
#include "FileReader.h"

#include <thread>
#include <functional>

#include "Model.h"
#include "Light.h"

glm::vec3 g_CameraPos;
glm::vec3 g_CameraLookAt;
glm::vec3 g_CameraUp;
float g_MoveSpeed = 10;

std::map<int, bool> keyMap;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	size_t count = keyMap.count(key);
	if (count == 0)
	{
		keyMap[key] = false;
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
		{
			keyMap[key] = true;
		}
	}
	else
	{
		bool isPressed = keyMap[key];
		if (isPressed && action == GLFW_RELEASE)
		{
			keyMap[key] = false;
		}
		else
		{
			keyMap[key] = true;
		}
	}

}

bool g_MouseButtonDown = false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			g_MouseButtonDown = true;
		}
		else
		{
			g_MouseButtonDown = false;
		}
	}
}

double g_LastMouseX = 0;
double g_LastMouseY = 0;
double g_MouseX = 0; 
double g_MouseY = 0;
double g_DeltaX;
double g_DeltaY;
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	g_LastMouseX = g_MouseX;
	g_LastMouseY = g_MouseY;
	g_MouseX = xpos;
	g_MouseY = ypos;
	g_DeltaX = g_MouseX - g_LastMouseX;
	g_DeltaY = g_MouseY - g_LastMouseY;
}

void UpdateInpute(float dTime)
{
	glm::vec3 lookDir = g_CameraLookAt - g_CameraPos;
	glm::vec3 lookRight= glm::cross(g_CameraUp, lookDir);
	glm::vec3 moveDir = glm::vec3(0);
	if (keyMap[GLFW_KEY_A])
	{
		moveDir += glm::normalize(lookRight);
	}
	if (keyMap[GLFW_KEY_D])
	{
		moveDir -= glm::normalize(lookRight);
	}
	if (keyMap[GLFW_KEY_W])
	{
		moveDir += glm::normalize(lookDir);
	}
	if (keyMap[GLFW_KEY_S])
	{
		moveDir -= glm::normalize(lookDir);
	}
	if (keyMap[GLFW_KEY_Q])
	{
		moveDir += glm::normalize(g_CameraUp);
	}
	if (keyMap[GLFW_KEY_E])
	{
		moveDir -= glm::normalize(g_CameraUp);
	}
	if (keyMap[GLFW_KEY_LEFT_SHIFT])
	{
		moveDir *= 5;
	}
	moveDir *= (dTime * g_MoveSpeed);
	g_CameraPos += moveDir;
	g_CameraLookAt += moveDir;


	if (g_MouseButtonDown)
	{
		glm::vec3 tempUp = glm::cross(lookRight, lookDir);

		float rotSpeed = 5.0f * dTime;
		float rotSpeedV = static_cast<float>(rotSpeed * g_DeltaY);
		float rotSpeedH = static_cast<float>(-rotSpeed * g_DeltaX);


		glm::mat4 rot1 = glm::rotate(glm::mat4(1.0), glm::radians(rotSpeedV), lookRight);
		glm::mat4 rot2 = glm::rotate(rot1, glm::radians(rotSpeedH), glm::vec3(0,1,0));

		lookDir = g_CameraLookAt - g_CameraPos;
		glm::vec4 lookVec4(lookDir, 0.0);
		lookVec4 = rot2 * lookVec4;
		lookVec4 = glm::normalize(lookVec4);
		g_CameraLookAt = g_CameraPos + glm::vec3(lookVec4.x, lookVec4.y, lookVec4.z);
	}
	g_CameraUp = glm::vec3(0,1,0);

	g_DeltaX = 0;
	g_DeltaY = 0;
}

void InitWindow(GLFWwindow** ppWindow, int width, int height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	*ppWindow = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);

	glfwSetKeyCallback(*ppWindow, key_callback); 
	glfwSetMouseButtonCallback(*ppWindow, mouse_button_callback); 
	glfwSetCursorPosCallback(*ppWindow, cursor_position_callback);
		
}

void DestroyWindow(GLFWwindow* pWindow)
{
	while (!glfwWindowShouldClose(pWindow)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(pWindow);

	glfwTerminate();
}

int main() {
	GLFWwindow* pWindow = nullptr;
	InitWindow(&pWindow, 1920,1080); 

	GraphicSystem graphicSystem;
	graphicSystem.InitGraphicsSystem(pWindow);
	VkDevice device = graphicSystem.GetDevice();


	VkFormat swapChainFormat = graphicSystem.GetSwapChainFormat();
	uint32_t swapChainCount = graphicSystem.GetSwapChainCount();
	VkExtent2D swapChainExtent = graphicSystem.GetSwapChainExtent();
	VkRenderPass renderPass = graphicSystem.GetRenderPass();
	VkCommandPool commandPool = graphicSystem.GetCommandPool();

	std::vector<VkQueue>& queues = graphicSystem.GetQueues();
	VkSwapchainKHR swapChain = graphicSystem.GetSwapChain();
	std::vector<VkFramebuffer>& swapChainFrameBuffers = graphicSystem.GetSwapChainFrameBuffers();

	CommandBuffer commandBuffer;
	commandBuffer.Init(device, swapChainCount, commandPool);

	//===========================================================================================================================
	glm::mat4 lightMtx;
	glm::quat rotate; 
	glm::vec3 eulerAngles(90, 45, 0);

	Light* pLight1 = LightManager::GetInstance().CreateNewLight();
	pLight1->SetLightType(DIRECTIONAL_LIGHT);

	lightMtx = glm::translate(glm::mat4(1.0f), glm::vec3(0, 3, 0));// DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&local)) *;
	eulerAngles = glm::vec3(glm::radians(-45.0f), glm::radians(45.0f), 0);
	rotate = glm::quat(eulerAngles);// MakeQuat(glm::vec3(0, 0, 1), glm::vec3(-1, -1, -1));
	lightMtx = lightMtx * glm::mat4_cast(rotate);
	pLight1->SetLightLocalTransform(lightMtx);

	pLight1->SetLightColor(glm::vec3(1, 1, 1));
	pLight1->SetLightIntensity(5);


	Light* pLight2 = LightManager::GetInstance().CreateNewLight();
	pLight2->SetLightType(POINT_LIGHT);

	lightMtx = glm::translate(glm::mat4(1.0f), glm::vec3(0, 3, 0));
	pLight2->SetLightLocalTransform(lightMtx);

	pLight2->SetLightRange(4);
	pLight2->SetLightColor(glm::vec3(0, 1, 0));
	pLight2->SetLightIntensity(10);

	Light* pLight3 = LightManager::GetInstance().CreateNewLight();
	pLight3->SetLightType(SPOT_LIGHT);

	lightMtx = glm::translate(glm::mat4(1.0f), glm::vec3(2, 1, 0));// DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&local)) *;
	eulerAngles = glm::vec3(glm::radians(-90.0f), 0, 0);
	rotate = glm::quat(eulerAngles);// MakeQuat(glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));
	lightMtx = lightMtx * glm::mat4_cast(rotate);
	pLight3->SetLightLocalTransform(lightMtx);

	pLight3->SetLightRange(2);
	pLight3->SetLightColor(glm::vec3(0, 0, 1));
	pLight3->SetLightIntensity(100);
	pLight3->SetOuterConeAngle(glm::radians(45.0f));
	pLight3->SetInnerConeAngle(glm::radians(30.0f));


	float x1 = pLight1->GetLightDir().x;
	float y1 = pLight1->GetLightDir().y;
	float z1 = pLight1->GetLightDir().z;

	float x3 = pLight3->GetLightDir().x;
	float y3 = pLight3->GetLightDir().y;
	float z3 = pLight3->GetLightDir().z;

	//===========================================================================================================================
	Model sponza;
	sponza.CreateModel("Models/Sponza/glTF/Sponza.gltf", "Models/Sponza/glTF", &graphicSystem);
	Model normalTangentTest;
	normalTangentTest.CreateModel("Models/NormalTangentTest/glTF/NormalTangentTest.gltf", "Models/NormalTangentTest/glTF", &graphicSystem);
	Model normalTangentMirrorTest;
	normalTangentMirrorTest.CreateModel("Models/NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf", "Models/NormalTangentMirrorTest/glTF", &graphicSystem);
	
	Model test;
	test.CreateModel("Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", "Models/MetalRoughSpheres/glTF", &graphicSystem);

	Model alphaBlendModeTest;
	alphaBlendModeTest.CreateModel("Models/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf", "Models/AlphaBlendModeTest/glTF", &graphicSystem);

	Model boomBoxWithAxes;
	boomBoxWithAxes.CreateModel("Models/BoomBoxWithAxes/glTF/BoomBoxWithAxes.gltf", "Models/BoomBoxWithAxes/glTF", &graphicSystem);

	Model boomBox;
	boomBox.CreateModel("Models/BoomBox/glTF/BoomBox.gltf", "Models/BoomBox/glTF", &graphicSystem);

	Model kko;
	kko.CreateModel("Models/Kko/sprits2.gltf", "Models/Kko", &graphicSystem);

	Model lightTest;
	lightTest.CreateModel("Models/LightTest/LightTest.gltf", "Models/LightTest", &graphicSystem);

	Model damagedHelmet;
	damagedHelmet.CreateModel("Models/DamagedHelmet/glTF/DamagedHelmet.gltf", "Models/DamagedHelmet/glTF/", &graphicSystem);

	Model cube;
	cube.CreateModel("Models/Cube/glTF/Cube.gltf", "Models/Cube/glTF/", &graphicSystem);

	normalTangentTest.SetTranslate(glm::vec3(1, 1, 0));
	normalTangentTest.SetScale(glm::vec3(0.2, 0.2, 0.2));

	normalTangentMirrorTest.SetTranslate(glm::vec3(0, 1, 0));
	normalTangentMirrorTest.SetScale(glm::vec3(0.2, 0.2, 0.2));

	alphaBlendModeTest.SetTranslate(glm::vec3(2.5, 0.75, 0));
	alphaBlendModeTest.SetScale(glm::vec3(0.2, 0.2, 0.2));

	test.SetTranslate(glm::vec3(-1, 1, 0));
	test.SetScale(glm::vec3(0.05, 0.05, 0.05));

	alphaBlendModeTest.SetTranslate(glm::vec3(2.5, 0.75, 0));
	alphaBlendModeTest.SetScale(glm::vec3(0.2, 0.2, 0.2));

	boomBoxWithAxes.SetTranslate(glm::vec3(-2, 1, 0));
	boomBoxWithAxes.SetScale(glm::vec3(20, 20, 20));

	boomBox.SetTranslate(glm::vec3(-3, 1, 0));
	boomBox.SetScale(glm::vec3(20, 20, 20));

	//kko.SetTranslate(glm::vec3(-2, 1, 0));
	kko.SetScale(glm::vec3(0.1, 0.1, 0.1));

	lightTest.SetTranslate(glm::vec3(15, 0, 0));

	damagedHelmet.SetTranslate(glm::vec3(-4, 1, 0));
	damagedHelmet.SetScale(glm::vec3(0.2, 0.2, 0.2));

	cube.SetTranslate(glm::vec3(-5, 1, 0));
	cube.SetScale(glm::vec3(0.1, 0.1, 0.1));
	//===================================================================================

	std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{ {-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}
	};
	std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};
	Model testModel1;
	testModel1.CreateModel("Texture/test.jpg",
		reinterpret_cast<void*>(vertices.data()), vertices.size(),
		reinterpret_cast<void*>(indices.data()), indices.size(),
		&graphicSystem);
	testModel1.SetTranslate(glm::vec3(-2, 1, -1));
	testModel1.SetScale(glm::vec3(1, 1, 1));

	Model testModel2;
	testModel2.CreateModel("Texture/IBLTestBrdf.dds",
		reinterpret_cast<void*>(vertices.data()), vertices.size(),
		reinterpret_cast<void*>(indices.data()), indices.size(),
		&graphicSystem);
	testModel2.SetTranslate(glm::vec3(2, 1, -1));
	testModel2.SetScale(glm::vec3(1, 1, 1));

	g_CameraPos = glm::vec3(1, 1, 0);
	g_CameraLookAt = glm::vec3(0, 1, 0);
	g_CameraUp = glm::vec3(0, 1, 0);

	g_MoveSpeed = 2.0f;

	graphicSystem.GetCamera().cameraPos = g_CameraPos;
	graphicSystem.GetCamera().cameraLookAt = g_CameraLookAt;
	graphicSystem.GetCamera().cameraUp = g_CameraUp;

	graphicSystem.GetCamera().viewMtx = glm::lookAt(graphicSystem.GetCamera().cameraPos, graphicSystem.GetCamera().cameraLookAt, graphicSystem.GetCamera().cameraUp);
	graphicSystem.GetCamera().projMtx = glm::perspective(glm::radians(45.0f), graphicSystem.GetSwapChainAspect(), 0.1f, 10000.0f);
	graphicSystem.GetCamera().projMtx[1][1] *= -1;

	for (uint32_t i = 0; i < swapChainCount; i++)
	{
		commandBuffer.Begin(i);

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = swapChainFrameBuffers[i];
		renderPassBeginInfo.renderArea.offset = { 0,0 };
		renderPassBeginInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue clearDepth = { 1.0f, 0.0f };
		std::vector<VkClearValue> clearColors = { clearColor , clearDepth };
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
		renderPassBeginInfo.pClearValues = clearColors.data();


		VkCommandBuffer cmdBuf = commandBuffer.GetCommandBuffer(i);
		vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		sponza.Draw(cmdBuf, i);
		normalTangentTest.Draw(cmdBuf, i);
		normalTangentMirrorTest.Draw(cmdBuf, i);
		test.Draw(cmdBuf, i); 
		alphaBlendModeTest.Draw(cmdBuf, i);
		boomBoxWithAxes.Draw(cmdBuf, i);
		boomBox.Draw(cmdBuf, i);

		testModel1.Draw(cmdBuf, i);
		testModel2.Draw(cmdBuf, i);

		kko.Draw(cmdBuf, i); 
		
		lightTest.Draw(cmdBuf, i);

		damagedHelmet.Draw(cmdBuf, i);

		cube.Draw(cmdBuf, i);

		vkCmdEndRenderPass(cmdBuf);

		commandBuffer.End(i);
	}

	std::vector<VkSemaphore> imageAvailableSemaphores, renderFinishedSemaphores;
	std::vector<VkFence> imageFences;
	imageAvailableSemaphores.resize(swapChainCount);
	renderFinishedSemaphores.resize(swapChainCount);
	imageFences.resize(swapChainCount);

	for (uint32_t i = 0; i < swapChainCount; i++)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(device, &fenceInfo, nullptr, &imageFences[i]);
		vkResetFences(device, 1, &imageFences[i]);
	}
	uint64_t currentFrame = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto lastTime = startTime;
	while (!glfwWindowShouldClose(pWindow))
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

		glfwPollEvents();
		UpdateInpute(dTime);

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame % swapChainCount], VK_NULL_HANDLE, &imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame % swapChainCount] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffers[] = { commandBuffer.GetCommandBuffer(imageIndex) };
		submitInfo.pCommandBuffers = commandBuffers;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame % swapChainCount] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		{
			graphicSystem.GetCamera().cameraPos = g_CameraPos;
			graphicSystem.GetCamera().cameraLookAt = g_CameraLookAt;
			graphicSystem.GetCamera().cameraUp = g_CameraUp;
			graphicSystem.GetCamera().viewMtx = glm::lookAt(g_CameraPos, g_CameraLookAt, g_CameraUp);
			
			normalTangentTest.Update(imageIndex);

			normalTangentMirrorTest.Update(imageIndex);

			alphaBlendModeTest.Update(imageIndex);

			sponza.Update(imageIndex);

			boomBoxWithAxes.Update(imageIndex);

			boomBox.Update(imageIndex);

			test.Update(imageIndex);

			kko.Update(imageIndex);

			damagedHelmet.Update(imageIndex);

			cube.Update(imageIndex);
			
			lightTest.Update(imageIndex);
			testModel1.Update(imageIndex);
			testModel2.Update(imageIndex);
		}

		vkQueueSubmit(queues[0], 1, &submitInfo, imageFences[currentFrame % swapChainCount]);

		if (currentFrame > 0)
		{
			vkWaitForFences(device, 1, &imageFences[(currentFrame - 1) % swapChainCount], VK_TRUE, UINT64_MAX);
			vkResetFences(device, 1, &imageFences[(currentFrame - 1) % swapChainCount]);
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(queues[1], &presentInfo);

		currentFrame++;
		lastTime = currentTime;
	}

	vkDeviceWaitIdle(device);


	sponza.Finalize();
	normalTangentTest.Finalize();
	normalTangentMirrorTest.Finalize(); 
	alphaBlendModeTest.Finalize();
	boomBoxWithAxes.Finalize();
	boomBox.Finalize();
	test.Finalize();
	kko.Finalize();
	damagedHelmet.Finalize();
	cube.Finalize();
	lightTest.Finalize();
	testModel1.Finalize();
	testModel2.Finalize();

	commandBuffer.Finalize();

	for (VkSemaphore semaphore : imageAvailableSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}
	for (VkSemaphore semaphore : renderFinishedSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}
	for (VkFence fence : imageFences)
	{
		vkDestroyFence(device, fence, nullptr);
	}

	for (VkFramebuffer framebuffer : swapChainFrameBuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	graphicSystem.Finalize();
	TextureManager::GetInstance().Finalize();

	DestroyWindow(pWindow);

	return 0;
}