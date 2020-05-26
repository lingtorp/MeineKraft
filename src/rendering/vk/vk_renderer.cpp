#include "vk_renderer.hpp"
#include "vk_utils.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_RIGHT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../nodes/model.hpp"

namespace VkUtil {
  static VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription description = {};
    description.binding = 0;
    description.stride = sizeof(Vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return description;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  get_attribute_descriptions() {
    std::array<VkVertexInputAttributeDescription, 2> descriptions = {};

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descriptions[0].offset = offsetof(Vertex, position);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    descriptions[1].offset = offsetof(Vertex, tex_coord);

    return descriptions;
  }
}

struct ShaderBundle {
  ShaderBundle() = default;
  ShaderBundle(VkDevice device, const std::string &vertex_filepath, const std::string &fragment_filepath) {
    // TODO: Check if .spv ending and do the correct thing

    const auto vert_shader_code = Filesystem::read_file(Filesystem::base + "shaders/vert.spv");

    VkShaderModuleCreateInfo vert_shadermodule_ci = {};
    vert_shadermodule_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_shadermodule_ci.codeSize = vert_shader_code.size();
    vert_shadermodule_ci.pCode = reinterpret_cast<const uint32_t*>(vert_shader_code.data());
    VkShaderModule vert_shadermodule;
    vkCreateShaderModule(device, &vert_shadermodule_ci, nullptr, &vert_shadermodule);

    const auto frag_shader_code = Filesystem::read_file(Filesystem::base + "shaders/frag.spv");

    VkShaderModuleCreateInfo frag_shadermodule_ci = {};
    frag_shadermodule_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shadermodule_ci.codeSize = frag_shader_code.size();
    frag_shadermodule_ci.pCode = reinterpret_cast<const uint32_t*>(frag_shader_code.data());
    VkShaderModule frag_shadermodule;
    vkCreateShaderModule(device, &frag_shadermodule_ci, nullptr, &frag_shadermodule);

    /// Pipeline
    VkPipelineShaderStageCreateInfo vert_pipeline_stage_ci = {};
    vert_pipeline_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_pipeline_stage_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_pipeline_stage_ci.module = vert_shadermodule;
    vert_pipeline_stage_ci.pName = "main";
    vert_pipeline_stage_ci.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo frag_pipeline_stage_ci = {};
    frag_pipeline_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_pipeline_stage_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_pipeline_stage_ci.module = frag_shadermodule;
    frag_pipeline_stage_ci.pName = "main";
    frag_pipeline_stage_ci.pSpecializationInfo = nullptr;

    this->shader_stages_ci = {vert_pipeline_stage_ci, frag_pipeline_stage_ci};

    this->vertex_input_binding = VkUtil::get_binding_description();
    this->vertex_input_attribs = VkUtil::get_attribute_descriptions();

    this->vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    this->vertex_input_ci.vertexBindingDescriptionCount = 1;
    this->vertex_input_ci.pVertexBindingDescriptions = &this->vertex_input_binding;
    this->vertex_input_ci.vertexAttributeDescriptionCount = this->vertex_input_attribs.size();
    this->vertex_input_ci.pVertexAttributeDescriptions = this->vertex_input_attribs.data();

    // vkDestroyShaderModule(device, vert_shadermodule, nullptr);
    // vkDestroyShaderModule(device, frag_shadermodule, nullptr);
  };

  // Vulkan objects
  VkVertexInputBindingDescription vertex_input_binding = {};
  std::array<VkVertexInputAttributeDescription, 2> vertex_input_attribs = {};


  std::vector<VkPipelineShaderStageCreateInfo> shader_stages_ci = {};
  VkPipelineVertexInputStateCreateInfo vertex_input_ci = {};
};

// TODO: Cleanup all Vulkan objects
Renderer::~Renderer() {
  vkDeviceWaitIdle(device); // Wait for all operations to finish
  vkDestroyInstance(instance, nullptr);
  vkDestroyDevice(device, nullptr);
  vmaDestroyBuffer(vma_allocator, vertex_buffer, nullptr);
  vmaDestroyBuffer(vma_allocator, index_buffer, nullptr);

  for (size_t i = 0; i < uniform_buffers.size(); i++) {
    vmaDestroyBuffer(vma_allocator, uniform_buffers[i].handle, nullptr);
  }
}

bool Renderer::init() {
  Scene scene(Filesystem::base + "rsrcs/box-textured/", "box-textured.gltf");
  Mesh mesh = scene.models[0].mesh;
  std::string texture_filepath = scene.models[0].textures.front().filepath;

  // ----- Vulkan ------

  const std::vector<const char*> supported_layers = vk::utils::supported_validation_layers();
  const std::vector<const char*> supported_extensions = vk::utils::supported_instance_extensions();

  // Request some validation layers
  std::vector<const char*> validation_layers = {"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_RENDERDOC_Capture"};
  vk::utils::create_instance(sdl_window, &instance, validation_layers, supported_extensions);
  vk::utils::create_debug_messenger(instance);

  /// Setup surface
  if (!SDL_Vulkan_CreateSurface(sdl_window, instance, &surface)) {
    Log::error(SDL_GetError());
    Log::error("[SDL]: Failed to create Vulkan surface");
  }

  const uint32_t queue_family_idx = vk::utils::setup_device(instance, &physical_device, &device, surface, validation_layers);
  vkGetDeviceQueue(device, queue_family_idx, 0, &queue);

  /// Initialize Vulkan Memory Allocator (VMA)
  VmaAllocatorCreateInfo vma_ci = {};
  vma_ci.physicalDevice = physical_device;
  vma_ci.device = device;

  VK_CHECK(vmaCreateAllocator(&vma_ci, &vma_allocator));

  swapchain = vk::utils::create_swapchain(physical_device, device, surface);

  /// Shaders
  const auto shader = ShaderBundle(device, Filesystem::base + "shaders/vert.spv", Filesystem::base + "shaders/frag.spv");

  VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {};
  input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_ci.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchain.image_extent.width;
  viewport.height = (float)swapchain.image_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.image_extent;

  VkPipelineViewportStateCreateInfo viewport_ci = {};
  viewport_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_ci.viewportCount = 1;
  viewport_ci.pViewports = &viewport;
  viewport_ci.scissorCount = 1;
  viewport_ci.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer_ci = {};
  rasterizer_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_ci.depthClampEnable = VK_FALSE;
  rasterizer_ci.rasterizerDiscardEnable = VK_FALSE;
  rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer_ci.lineWidth = 1.0f;
  rasterizer_ci.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer_ci.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling_ci = {};
  multisampling_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling_ci.sampleShadingEnable = VK_FALSE;
  multisampling_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  // Color blending configurations
  VkPipelineColorBlendStateCreateInfo color_blending_ci = {};
  color_blending_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending_ci.logicOpEnable = VK_FALSE;
  color_blending_ci.attachmentCount = 1;
  color_blending_ci.pAttachments = &color_blend_attachment;

  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

  VkPipelineDynamicStateCreateInfo dynamics_state = {};
  dynamics_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamics_state.dynamicStateCount = 2;
  dynamics_state.pDynamicStates = dynamic_states;

  // Uniform buffer specification
  VkDescriptorSetLayoutBinding ubo_layout_binding = {};
  ubo_layout_binding.binding = 0;
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.descriptorCount = 1;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // Texture sampler descriptor binding
  VkDescriptorSetLayoutBinding sampler_layout_binding = {};
  sampler_layout_binding.binding = 1;
  sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::vector<VkDescriptorSetLayoutBinding> bindings = {ubo_layout_binding, sampler_layout_binding};

  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_ci = {};
  descriptor_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptor_set_layout_ci.bindingCount = bindings.size();
  descriptor_set_layout_ci.pBindings = bindings.data();

  VkDescriptorSetLayout descriptor_set_layout = {};
  VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_ci, nullptr, &descriptor_set_layout));

  // Uniform values specification
  VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
  pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_ci.setLayoutCount = 1;
  pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;

  VkPipelineLayout pipeline_layout = {};
  VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_ci, nullptr, &pipeline_layout));

  /// Render pass
  // Maps to one of the swapchain images
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = swapchain.image_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  // Before rendering: glClear(GL_COLOR_BIT) basically
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // After rendering
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Before rendering
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // After rendering

  VkAttachmentReference color_attachment_ref = {};
  // Index to the VkAttachmentDescription struct
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // NOTE: subpasses in a render pass automatically take care of image layout transitions.
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.flags = 0;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // NOTE: Special value VK_SUBPASS_EXTERNAL refers to the implicit subpass
  dependency.dstSubpass = 0; // Index to subpass
  // Operations to wait for
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_ci = {};
  render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_ci.flags = 0;
  render_pass_ci.attachmentCount = 1;
  render_pass_ci.pAttachments = &color_attachment;
  render_pass_ci.subpassCount = 1;
  render_pass_ci.pSubpasses = &subpass;
  render_pass_ci.dependencyCount = 1;
  render_pass_ci.pDependencies = &dependency;

  VkRenderPass render_pass = {};
  VK_CHECK(vkCreateRenderPass(device, &render_pass_ci, nullptr, &render_pass));

  /// Graphics pipeline
  VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {};
  graphics_pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphics_pipeline_ci.stageCount = 2;
  graphics_pipeline_ci.pStages = shader.shader_stages_ci.data();
  graphics_pipeline_ci.pVertexInputState = &shader.vertex_input_ci;
  graphics_pipeline_ci.pInputAssemblyState = &input_assembly_ci;
  graphics_pipeline_ci.pViewportState = &viewport_ci;
  graphics_pipeline_ci.pRasterizationState = &rasterizer_ci;
  graphics_pipeline_ci.pMultisampleState = &multisampling_ci;
  graphics_pipeline_ci.pDepthStencilState = nullptr;
  graphics_pipeline_ci.pColorBlendState = &color_blending_ci;
  graphics_pipeline_ci.pDynamicState = nullptr;

  graphics_pipeline_ci.layout = pipeline_layout;
  graphics_pipeline_ci.renderPass = render_pass;
  graphics_pipeline_ci.subpass = 0;

  VkPipeline graphics_pipeline = {};
  VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_ci, nullptr, &graphics_pipeline));

  /// Framebuffer
  std::vector<VkFramebuffer> swapchain_framebuffers(swapchain.image_views.size());
  for (size_t i = 0; i < swapchain_framebuffers.size(); i++) {
    VkImageView attachments[] = {swapchain.image_views[i]};

    VkFramebufferCreateInfo framebuffer_ci = {};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.renderPass = render_pass;
    framebuffer_ci.attachmentCount = 1;
    framebuffer_ci.pAttachments = attachments;
    framebuffer_ci.width = swapchain.image_extent.width;
    framebuffer_ci.height = swapchain.image_extent.height;
    framebuffer_ci.layers = 1;
    VK_CHECK(vkCreateFramebuffer(device, &framebuffer_ci, nullptr, &swapchain_framebuffers[i]));
  }

  /// Command pool & buffers
  VkCommandPoolCreateInfo cmd_pool_ci = {};
  cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_ci.queueFamilyIndex = queue_family_idx;
  cmd_pool_ci.flags = 0;
  VK_CHECK(vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &cmd_pool));

  command_buffers = std::vector<VkCommandBuffer>(swapchain_framebuffers.size());
  VkCommandBufferAllocateInfo command_alloc_info = {};
  command_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_alloc_info.commandPool = cmd_pool;
  command_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_alloc_info.commandBufferCount = (uint32_t)command_buffers.size();
  VK_CHECK(vkAllocateCommandBuffers(device, &command_alloc_info, command_buffers.data()));

  /// Upload vertices to the GPU
  vertex_buffer = vk_upload_data(mesh.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  index_buffer  = vk_upload_data(mesh.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  // Texture sampler setup
  texture_image_obj = vk::utils::create_texture_image(this, vma_allocator, texture_filepath);
  texture_image_view = vk::utils::create_image_view(device, texture_image_obj.handle, VK_FORMAT_R8G8B8A8_UNORM);
  texture_sampler = vk::utils::create_texture_sampler(device);

  // Initialize descriptor sets
  this->create_uniform_buffers(swapchain.image_views.size());
  descriptor_pool = vk::utils::create_descriptorset_pool(device, swapchain.image_views.size());
  descriptor_sets = this->create_descriptorsets(swapchain.image_views.size(), descriptor_set_layout);

  // Record draw commands into cmd buffer
  for (size_t i = 0; i < command_buffers.size(); i++) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;
    VK_CHECK(vkBeginCommandBuffer(command_buffers[i], &begin_info));

    VkRenderPassBeginInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swapchain_framebuffers[i];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain.image_extent;

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clearColor;

    /// Recording render pass
    vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer, offsets);

    vkCmdBindIndexBuffer(command_buffers[i], index_buffer, offsets[0], VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);

    vkCmdDrawIndexed(command_buffers[i], mesh.indices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffers[i]);

    VK_CHECK(vkEndCommandBuffer(command_buffers[i]));
  }

  VkSemaphoreCreateInfo semaphore_ci = {};
  semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, nullptr, &image_available_sem));
  VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, nullptr, &render_finished_sem));

  return true;
}

std::vector<VkDescriptorSet>
Renderer::create_descriptorsets(const size_t swapchain_image_count,
                                const VkDescriptorSetLayout layout) const {
  std::vector<VkDescriptorSet> sets(swapchain_image_count);
  std::vector<VkDescriptorSetLayout> layouts(swapchain_image_count, layout);

  VkDescriptorSetAllocateInfo alloc_info = {};
  alloc_info.descriptorPool = descriptor_pool;
  alloc_info.descriptorSetCount = sets.size();
  alloc_info.pSetLayouts = layouts.data();

  VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, sets.data()));

  // Initialize the descriptor sets
  for (size_t i = 0; i < swapchain_image_count; i++) {
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = uniform_buffers[i].handle;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> write_descriptors = {};
    write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptors[0].descriptorCount = 1;
    write_descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptors[0].dstSet = sets[i];
    write_descriptors[0].dstBinding = 0;
    write_descriptors[0].pBufferInfo = &buffer_info;

    // ----------------------------------------

    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = texture_image_view;
    image_info.sampler = texture_sampler;

    write_descriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptors[1].dstSet = sets[i];
    write_descriptors[1].dstBinding = 1;
    write_descriptors[1].dstArrayElement = 0;
    write_descriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptors[1].descriptorCount = 1;
    write_descriptors[1].pImageInfo = &image_info;

    vkUpdateDescriptorSets(device, write_descriptors.size(), write_descriptors.data(), 0, nullptr);
  }

  return sets;
}

void Renderer::create_uniform_buffers(const uint32_t count) {
  uniform_buffers.resize(count);
  const size_t size = sizeof(MVPUniform);
  const VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  const VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  const VmaAllocationCreateFlagBits mem_props = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  for (size_t i = 0; i < count; i++) {
    uniform_buffers[i] = vk::utils::create_buffer(vma_allocator, size, usage, mem_usage, mem_props);
  }
}

void Renderer::update_uniform_buffer(const uint32_t idx) {
  static auto start = std::chrono::high_resolution_clock::now();

  const auto current = std::chrono::high_resolution_clock::now();
  const float time = std::chrono::duration<float, std::chrono::seconds::period>(start - current).count();

  MVPUniform ubo = {};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), swapchain.image_extent.width / (float)swapchain.image_extent.height, 0.1f, 10.0f);
  std::memcpy(uniform_buffers[idx].info.pMappedData, &ubo, sizeof(MVPUniform));
}

/// Draw frame
void Renderer::render_frame() {
  // Acquire image from the swapchain
  uint32_t image_idx = 0;
  // 3rd parameter: timeout (max disables it)
  vkAcquireNextImageKHR(device, swapchain.handle, std::numeric_limits<uint64_t>::max(), image_available_sem, VK_NULL_HANDLE, &image_idx);

  update_uniform_buffer(image_idx);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkSemaphore wait_semaphores[] = {image_available_sem};
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers[image_idx];

  // Config. semaphores to signal once the cmd buffer(s) have finished execution
  VkSemaphore signal_semaphores[] = {render_finished_sem};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;
  VkSwapchainKHR swapchains[] = {swapchain.handle};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &image_idx;

  vkQueuePresentKHR(queue, &present_info);

  vkQueueWaitIdle(queue);
}

// FIXME: Maybe extract cpy_buffer and create_buffer functionality
template<typename T>
VkBuffer Renderer::vk_upload_data(const std::vector<T> &data, const VkBufferUsageFlagBits usage) {
  // Staging buffer
  VkBufferCreateInfo staging_buffer_ci = {};
  staging_buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  staging_buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  staging_buffer_ci.size = data.size() * sizeof(T);
  staging_buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo staging_alloc_ci = {};
  staging_alloc_ci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  staging_alloc_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo staging_alloc_info = {};
  VmaAllocation staging_allocation = {};
  VkBuffer staging_buffer = {};
  VK_CHECK(vmaCreateBuffer(vma_allocator, &staging_buffer_ci,
                            &staging_alloc_ci, &staging_buffer,
                            &staging_allocation, &staging_alloc_info));

  assert(staging_alloc_info.pMappedData);
  std::memcpy(staging_alloc_info.pMappedData, data.data(), data.size() * sizeof(data[0]));

  // GPU buffer
  VkBufferCreateInfo gpu_ci = {};
  gpu_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  gpu_ci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  gpu_ci.usage |= usage;
  gpu_ci.size = data.size() * sizeof(data[0]);
  gpu_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo gpu_alloc_ci = {};
  gpu_alloc_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VmaAllocationInfo gpu_alloc_info = {};
  VmaAllocation gpu_allocation = {};
  VkBuffer gpu_buffer = {};
  VK_CHECK(vmaCreateBuffer(vma_allocator, &gpu_ci, &gpu_alloc_ci, &gpu_buffer, &gpu_allocation, &gpu_alloc_info));

  // Copy from staging to gpu buffer
  VkCommandBufferAllocateInfo cmd_buffer_ci = {};
  cmd_buffer_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_buffer_ci.commandPool = cmd_pool;
  cmd_buffer_ci.commandBufferCount = 1;
  cmd_buffer_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VkCommandBuffer cmd_buffer = {};
  VK_CHECK(vkAllocateCommandBuffers(device, &cmd_buffer_ci, &cmd_buffer));

  VkCommandBufferBeginInfo cmd_begin_info = {};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(cmd_buffer, &cmd_begin_info);

  VkBufferCopy copy_region = {};
  copy_region.size = data.size() * sizeof(data[0]);

  vkCmdCopyBuffer(cmd_buffer, staging_buffer, gpu_buffer, 1, &copy_region);

  vkEndCommandBuffer(cmd_buffer);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;

  vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);
  vmaDestroyBuffer(vma_allocator, staging_buffer, staging_allocation);

  return gpu_buffer;
}
