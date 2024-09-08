#ifndef VULKRAFT_APPLICATION_H
#define VULKRAFT_APPLICATION_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <set>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "camera/camera.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunkDrawing.h"

#include "player/player.hpp"
#include "raycast/trace-ray.hpp"

#include <ThreadedLoggerForCPP/LoggerThread.hpp>

#include <ThreadedLoggerForCPP/LoggerFileSystem.hpp>
#include <ThreadedLoggerForCPP/LoggerGlobals.hpp>

#include <game_performance_profiler.hpp>

#include "globals.h"
#include <vulkan/vulkan.hpp>

#include "utils/structs.hpp"
const int MAX_FRAMES_IN_FLIGHT = 2;
const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
const std::string TEXTURE_PATH = "textures/blocks.png";

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
class VulkraftApplication {
public:
  void run();
  void updateVertexBuffer();

  void updateIndexBuffer();

  std::vector<BlockVertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<BlockVertex> waterVertices;
  std::vector<uint32_t> waterIndices;
  bool cursorEnabled = true;

  GLFWwindow *window;

private:
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  VkDevice device;

  VkQueue graphicsQueue;
  VkQueue presentQueue;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkCommandPool commandPool;

  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;

  uint32_t mipLevels;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  std::vector<VkBuffer> vertexBuffer;
  std::vector<VkDeviceMemory> vertexBufferMemory;
  std::vector<VkBuffer> indexBuffer;
  std::vector<VkDeviceMemory> indexBufferMemory;

  std::vector<VkBuffer> waterVertexBuffer;
  std::vector<VkDeviceMemory> waterVertexBufferMemory;
  std::vector<VkBuffer> waterIndexBuffer;
  std::vector<VkDeviceMemory> waterIndexBufferMemory;

  int curBuffer = 0;

  std::vector<VkBuffer> vertexUniformBuffers;
  std::vector<VkDeviceMemory> vertexUniformBuffersMemory;
  std::vector<VkBuffer> fragmentUniformBuffers;
  std::vector<VkDeviceMemory> fragmentUniformBuffersMemory;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;

  glm::vec3 sunDir;

  bool framebufferResized = false;

  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);

  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator);

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void initWindow();
  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  void initVulkan();
  void createInstance();
  std::vector<const char *> getRequiredExtensions();
  bool checkValidationLayerSupport();
  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  void setupDebugMessenger();
  void createSurface();

  void pickPhysicalDevice();

  bool isDeviceSuitable(VkPhysicalDevice device);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
  VkSampleCountFlagBits getMaxUsableSampleCount();
  void createLogicalDevice();

  void createSwapChain();
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  void createImageViews();

  VkImageView createImageView(VkImage image, VkFormat format,
                              VkImageAspectFlags aspectFlags,
                              uint32_t mipLevels);

  void createRenderPass();
  void createDescriptorSetLayouts();
  void createDescriptorSetLayout();
  void createPipelines();
  void createGraphicsPipeline();

  static std::vector<char> readFile(const std::string &filename);

  VkShaderModule createShaderModule(const std::vector<char> &code);

  void createFramebuffers();
  void createCommandPool();
  void createColorResources();

  void createDepthResources();

  VkFormat findDepthFormat();

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  bool hasStencilComponent(VkFormat format);

  void createTextureImage();
  void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth,
                       int32_t texHeight, uint32_t mipLevels);

  void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                   VkSampleCountFlagBits numSamples, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image,
                   VkDeviceMemory &imageMemory);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  void createTextureImageView();
  void createTextureSampler();

  void createVertexBuffer();

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  VkCommandBuffer beginSingleTimeCommands();

  void createIndexBuffer();

  void createUniformBuffers();

  void createDescriptorPool();

  void createDescriptorSets();
  void createDescriptorSet();

  void createCommandBuffers();

  void createSyncObjects();
  void mainLoop();

  void drawFrame();
  void updateUniformBuffer(uint32_t currentImage);

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                           int currentFrame);
  void recreateSwapChain();

  void cleanupSwapChain();

  void cleanup();

  //// ===== Custom control functions =====

  // Moves the sun in the higher position
  void setTimeNoon();

  // Moves the sun in the lower position
  void setTimeMidnight();
  // Toggles [cursorEnabled] and the ability to interact with the application
  void toggleCursor();
};
#endif // VULKRAFT_APPLICATION_H