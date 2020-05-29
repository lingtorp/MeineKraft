#pragma once

#include "vk_debug.hpp"
#include "../../util/logging.hpp"
#include "vk_objects.hpp"
#include "../../util/filesystem.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const bool validation_enabled = true;

struct MVPUniform {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

// TODO: Document
struct Renderer {
  SDL_Window *sdl_window = nullptr;
  VkInstance instance = {};
  VkSurfaceKHR surface = {};
  VkPhysicalDevice physical_device = {};

  VkCommandPool cmd_pool = {};
  VkDevice device = {};
  VkQueue queue = {};
  std::vector<VkCommandBuffer> command_buffers = {};

  VkSemaphore image_available_sem = {};
  VkSemaphore render_finished_sem = {};

  VmaAllocator vma_allocator;
  VkBuffer vertex_buffer = {};
  VkBuffer index_buffer = {};

  VkSwapchainObject swapchain = {};

  std::vector<VkBufferObject> uniform_buffers = {};

  VkDescriptorPool descriptor_pool = {};
  std::vector<VkDescriptorSet> descriptor_sets = {};

  VkImageObject texture_image_obj = {};
  VkImageView texture_image_view = {};
  VkSampler texture_sampler = {};

  Renderer() {}
  ~Renderer();

  bool init();

  /// Draw frame
  void render_frame();

  std::vector<VkDescriptorSet> create_descriptorsets(const size_t swapchain_image_count, const VkDescriptorSetLayout layout) const;

  void create_uniform_buffers(const uint32_t count);
  void update_uniform_buffer(const uint32_t idx);

  template<typename T>
  VkBuffer vk_upload_data(const std::vector<T> &data, const VkBufferUsageFlagBits usage);
};
