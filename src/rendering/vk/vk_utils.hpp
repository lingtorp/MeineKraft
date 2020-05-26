#pragma once

#include "vk_objects.hpp"

struct Renderer;

// TODO: Document
namespace vk::utils {
  /// Debug utils EXT
  void create_debug_messenger(VkInstance instance);

  std::vector<const char*> supported_validation_layers();

  std::vector<const char*> supported_instance_extensions();

  // ---------------------------------------------------------------------------------------
  // Various helpers for Vulkan objects
  // ---------------------------------------------------------------------------------------

  VkDescriptorPool create_descriptorset_pool(VkDevice device, const size_t swapchain_image_count);

  VkSampler create_texture_sampler(VkDevice device);

  VkImageView create_image_view(VkDevice device, VkImage image, VkFormat fmt);

  VkImageObject create_image(VmaAllocator allocator, const VkImageCreateInfo *image_ci, const VmaMemoryUsage allocation_usage);

  VkImageObject create_texture_image(struct Renderer* renderer, VmaAllocator allocator, const std::string &texture_filepath);

  VkBufferObject create_buffer(VmaAllocator allocator, const size_t size,
                     const VkBufferUsageFlagBits usage,
                     const VmaMemoryUsage mem_usage,
                     const VmaAllocationCreateFlagBits mem_props);

  // ---------------------------------------------------------------------------------------
  // Command helpers for Vulkan objects
  // ---------------------------------------------------------------------------------------

  void cmd_transition_image_layout(struct Renderer* renderer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

  void cmd_copy_buffer_to_image(struct Renderer* renderer, VkBuffer buffer, VkImage image, const uint32_t w, const uint32_t h);

  void cmd_copy_buffer(struct Renderer* renderer, VkBuffer dst, VkBuffer src, const size_t size);

  VkCommandBuffer begin_once_cmd(struct Renderer* renderer);

  void end_once_cmd(struct Renderer* renderer, VkCommandBuffer cmd_buffer);

  // ---------------------------------------------------------------------------------------
  // Initialization routines for Vulkan
  // ---------------------------------------------------------------------------------------

  void create_instance(SDL_Window* window, VkInstance* instance, const std::vector<const char*>& validation_layers, const std::vector<const char*>& instance_extensions);

  /// Sets up physical device and return queue family index to a queue capable of graphics
  uint32_t setup_device(VkInstance instance, VkPhysicalDevice* physical_device, VkDevice* device, VkSurfaceKHR surface, const std::vector<const char*>& validation_layers);

  /// Swapchain creation and configuration
  VkSwapchainObject create_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface);
} // namespace vk::utils
