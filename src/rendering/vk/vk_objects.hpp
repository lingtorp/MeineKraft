#pragma once

#include <vector>
#include <cassert>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.hpp"

#include "../../util/logging.hpp"

inline void VK_CHECK(const VkResult result) {
  if (result != VK_SUCCESS) {
    Log::error("VK_CHECK: " + std::to_string(result));
  }
  assert(result == VK_SUCCESS);
}

struct VkBufferObject {
  VkBuffer handle;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};

struct VkImageObject {
  VkImage handle;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};

struct VkSwapchainObject {
  VkSwapchainKHR handle;
  VkExtent2D image_extent;
  VkFormat   image_format;
  std::vector<VkImage>     images = {};
  std::vector<VkImageView> image_views = {};
};
