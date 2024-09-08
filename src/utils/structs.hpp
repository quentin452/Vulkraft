#ifndef STRUCTS_H
#define STRUCTS_H
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <optional>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct VertexUniformBufferObject {
  alignas(16) glm::mat4 mvpMat;
  alignas(16) glm::mat4 mMat;
  alignas(16) glm::mat4 nMat;
};

struct FragmentUniformBufferObject {
  alignas(16) glm::vec3 sunLightDir;
  alignas(16) glm::vec3 sunLightCol;
  alignas(16) glm::vec3 moonLightDir;
  alignas(16) glm::vec3 moonLightCol;
  alignas(16) glm::vec3 eyePos;
  alignas(16) glm::vec3 eyeDir;
  alignas(16) glm::vec2 ambientParams;
};
#endif // STRUCTS_H