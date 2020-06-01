#include "vk_utils.hpp"
#include "vk_debug.hpp"
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../../include/stb/stb_image.h"

#include "vk_renderer.hpp"

namespace vk::utils {
  /// Debug utils EXT
  void create_debug_messenger(VkInstance instance) {
    VkDebugUtilsMessengerEXT debug_messenger = {};

    VkDebugUtilsMessengerCreateInfoEXT debug_ci = {};
    debug_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_ci.pfnUserCallback = vk_debug_callback;

    VkResult create_debug_utils_messenger = [&]() -> VkResult {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
      if (func) {
        return func(instance, &debug_ci, nullptr, &debug_messenger);
      }
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }();
    VK_CHECK(create_debug_utils_messenger);
  }

  std::vector<const char*> supported_instance_extensions() {
    uint32_t count = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    Log::info("# Instance extensions supported: " + std::to_string(count));

    std::vector<VkExtensionProperties> extension_props(count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_props.data()));

    bool found_extension = false;
    std::vector<const char*> extensions;
    for (VkExtensionProperties prop : extension_props) {
      if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0) {
        found_extension = true;
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      }
      Log::info(prop.extensionName);
    }

    assert(found_extension);

    return extensions;
  }

  // Returns all the supported layers
  std::vector<const char*> supported_validation_layers() {
    std::vector<const char*> layers;

    uint32_t count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
    Log::info("# Validation layers supported: " + std::to_string(count));

    if (count == 0) {
      return layers;
    }

    std::vector<VkLayerProperties> layer_properties(count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&count, &layer_properties[0]));


    for (const VkLayerProperties layer_property : layer_properties) {
      layers.push_back(layer_property.layerName);
      Log::info("[Validation layer]: " + std::string(layer_property.layerName));
    }

    return layers;
  }

  // ---------------------------------------------------------------------------------------
  // Creation helpers for Vulkan objects
  // ---------------------------------------------------------------------------------------

  VkDescriptorPool create_descriptorset_pool(VkDevice device, const size_t swapchain_image_count) {
    std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = swapchain_image_count;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = swapchain_image_count;

    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.poolSizeCount = pool_sizes.size();
    pool_ci.pPoolSizes = pool_sizes.data();
    pool_ci.maxSets = swapchain_image_count;

    VkDescriptorPool pool = {};
    VK_CHECK(vkCreateDescriptorPool(device, &pool_ci, nullptr, &pool));

    return pool;
  }


  VkSampler create_texture_sampler(VkDevice device) {
    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.magFilter = VK_FILTER_LINEAR;
    sampler_ci.minFilter = VK_FILTER_LINEAR;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.anisotropyEnable = VK_TRUE;
    sampler_ci.maxAnisotropy = 16;
    sampler_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_ci.mipLodBias = 0.0f;
    sampler_ci.minLod = 0.0f;
    sampler_ci.maxLod = 0.0f;

    VkSampler sampler = {};
    VK_CHECK(vkCreateSampler(device, &sampler_ci, nullptr, &sampler));
    return sampler;
  }

  VkImageView create_image_view(VkDevice device, VkImage image, VkFormat fmt) {
    VkImageViewCreateInfo imageview_ci = {};
    imageview_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageview_ci.format = fmt;
    imageview_ci.image = image;
    imageview_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageview_ci.subresourceRange.baseMipLevel = 0;
    imageview_ci.subresourceRange.levelCount = 1;
    imageview_ci.subresourceRange.baseArrayLayer = 0;
    imageview_ci.subresourceRange.layerCount = 1;

    VkImageView imageview = {};
    VK_CHECK(vkCreateImageView(device, &imageview_ci, nullptr, &imageview));
    return imageview;
  }

  VkImageObject create_image(VmaAllocator allocator, const VkImageCreateInfo *image_ci, const VmaMemoryUsage allocation_usage) {
    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.usage = allocation_usage;
    VkImageObject obj = {};
    vmaCreateImage(allocator, image_ci, &alloc_ci, &obj.handle, &obj.allocation, &obj.info);
    return obj;
  }

  VkImageObject create_texture_image(struct Renderer* renderer, VmaAllocator allocator, const std::string &texture_filepath) {
    int width, height, channels;
    stbi_uc *pixels = stbi_load(texture_filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    assert(channels == 4);
    if (!pixels) {
      Log::error("Could not load texture: " + texture_filepath);
      return {};
    }

    // Create staging buffer
    const size_t staging_buffer_size = channels * width * height;
    VkBufferObject staging_buffer = vk::utils::create_buffer(allocator, staging_buffer_size,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                                    VMA_ALLOCATION_CREATE_MAPPED_BIT);
    assert(staging_buffer.info.pMappedData);
    std::memcpy(staging_buffer.info.pMappedData, pixels, staging_buffer_size);
    stbi_image_free(pixels);

    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.extent.width = static_cast<uint32_t>(width);
    image_ci.extent.height = static_cast<uint32_t>(height);
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;

    VkImageObject obj = vk::utils::create_image(allocator, &image_ci, VMA_MEMORY_USAGE_GPU_ONLY);

    // Transfer to image
    vk::utils::cmd_transition_image_layout(renderer, obj.handle, image_ci.initialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk::utils::cmd_copy_buffer_to_image(renderer, staging_buffer.handle, obj.handle, width, height);
    vk::utils::cmd_transition_image_layout(renderer, obj.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(allocator, staging_buffer.handle, staging_buffer.allocation);

    return obj;
  }

  VkBufferObject create_buffer(VmaAllocator allocator, const size_t size,
                     const VkBufferUsageFlagBits usage,
                     const VmaMemoryUsage mem_usage,
                     const VmaAllocationCreateFlagBits mem_props) {
    VkBufferObject obj = {};

    VkBufferCreateInfo buffer_ci = {};
    buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.usage = usage;
    buffer_ci.size = size;
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.usage = mem_usage;
    alloc_ci.flags = mem_props;

    VK_CHECK(vmaCreateBuffer(allocator, &buffer_ci, &alloc_ci, &obj.handle, &obj.allocation, &obj.info));

    return obj;
  }

  // ---------------------------------------------------------------------------------------
  // Command helpers for Vulkan objects
  // ---------------------------------------------------------------------------------------

  void cmd_transition_image_layout(struct Renderer* renderer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer cmd_buffer = vk::utils::begin_once_cmd(renderer);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkPipelineStageFlags srcStage = {};
    VkPipelineStageFlags dstStage = {};
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      // Transfering from staging to GPU local buffer/image
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      // Transfering to readable by frag shader
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
      Log::error("Invalid image layout transition");
    }

    vkCmdPipelineBarrier(cmd_buffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vk::utils::end_once_cmd(renderer, cmd_buffer);
  }

  void cmd_copy_buffer_to_image(struct Renderer* renderer, VkBuffer buffer, VkImage image, const uint32_t w, const uint32_t h) {
    VkCommandBuffer cmd_buffer = vk::utils::begin_once_cmd(renderer);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {w, h, 1};

    // NOTE: Assuming that the dst has been transitioned into the optimal layout
    vkCmdCopyBufferToImage(cmd_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vk::utils::end_once_cmd(renderer, cmd_buffer);
  }

  void cmd_copy_buffer(struct Renderer* renderer, VkBuffer dst, VkBuffer src, const size_t size) {
    VkCommandBuffer cmd_buffer = vk::utils::begin_once_cmd(renderer);

    VkBufferCopy cpy = {};
    cpy.size = size;
    vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &cpy);

    vk::utils::end_once_cmd(renderer, cmd_buffer);
  }

  VkCommandBuffer begin_once_cmd(struct Renderer* renderer) {
    VkCommandBufferAllocateInfo cmd_buffer_alloc_ci = {};
    cmd_buffer_alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_ci.commandPool = renderer->cmd_pool;
    cmd_buffer_alloc_ci.commandBufferCount = 1;
    cmd_buffer_alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer cmd_buffer;
    VK_CHECK(vkAllocateCommandBuffers(renderer->device, &cmd_buffer_alloc_ci, &cmd_buffer));

    VkCommandBufferBeginInfo cmd_buffer_begin_ci = {};
    cmd_buffer_begin_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buffer_begin_ci.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_ci);

    return cmd_buffer;
  }

  void end_once_cmd(struct Renderer* renderer, VkCommandBuffer cmd_buffer) {
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;

    vkQueueSubmit(renderer->queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(renderer->queue);

    vkFreeCommandBuffers(renderer->device, renderer->cmd_pool, 1, &cmd_buffer);
  }

  // ---------------------------------------------------------------------------------------
  // Initialization routines for Vulkan
  // ---------------------------------------------------------------------------------------

  void create_instance(SDL_Window* window, VkInstance* instance, const std::vector<const char*> &validation_layers, const std::vector<const char*>& instance_extensions) {
    uint32_t extensions_count;
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, nullptr);

    std::vector<const char *> extensions(extensions_count);
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, extensions.data());
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

    for (const char *extension : extensions) {
      Log::info("Loading instance extension: " + std::string(extension));
    }

    for (const char *extension : instance_extensions) {
      Log::info("Loading instance extension: " + std::string(extension));
      extensions.push_back(extension);
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.pApplicationName = "vkVCT";

    /// Vulkan instance
    VkInstanceCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.enabledExtensionCount = extensions.size();
    ici.ppEnabledExtensionNames = extensions.data();
    ici.enabledLayerCount = validation_layers.size();
    ici.ppEnabledLayerNames = validation_layers.data();
    ici.pApplicationInfo = &app_info;

    VK_CHECK(vkCreateInstance(&ici, nullptr, instance));
  }

  /// Sets up physical device and return queue family index to a queue capable of graphics
  uint32_t setup_device(VkInstance instance, VkPhysicalDevice* physical_device, VkDevice* device, VkSurfaceKHR surface, const std::vector<const char*>& validation_layers) {
    uint32_t num_devices = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &num_devices, nullptr));

    std::vector<VkPhysicalDevice> devices(num_devices);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &num_devices, devices.data()));

    // FIXME: Just picks the first device found without actually looking for a suitable device
    *physical_device = devices.front();

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(*physical_device, &device_properties);
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
        device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      Log::info("Found a GPU");
    }

    /// Queue
    uint32_t num_queue_family = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*physical_device, &num_queue_family, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(num_queue_family);
    vkGetPhysicalDeviceQueueFamilyProperties(*physical_device, &num_queue_family, queue_families.data());

    int queue_family_index = -1;
    for (size_t i = 0; i < num_queue_family; i++) {
      if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
          queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        queue_family_index = i;
        Log::info("Found a queue with graphics & compute capabilities");
        break;
      }
    }

    if (queue_family_index < 0) {
      Log::error("Did not find a queue with graphics & compute capabilities"); exit(-1);
    }

    VkBool32 presentation_supported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(*physical_device, queue_family_index, surface, &presentation_supported);
    if (!presentation_supported) {
      Log::error("Presentation not supported by queue_family_index"); exit(-1);
    }

    /// Logical device setup
    VkDeviceQueueCreateInfo dqci = {};
    dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    dqci.queueFamilyIndex = queue_family_index;
    dqci.queueCount = 1;
    const float queue_prio = 1.0f;
    dqci.pQueuePriorities = &queue_prio;

    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pQueueCreateInfos = &dqci;
    dci.queueCreateInfoCount = 1;
    dci.pEnabledFeatures = &device_features;
    std::vector<const char *> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    dci.enabledExtensionCount = device_extensions.size();
    dci.ppEnabledExtensionNames = device_extensions.data();

    dci.enabledLayerCount = 0;
    if (!validation_layers.empty()) {
      dci.enabledLayerCount = validation_layers.size();
      dci.ppEnabledLayerNames = validation_layers.data();
    }

    VK_CHECK(vkCreateDevice(*physical_device, &dci, nullptr, device));

    return queue_family_index;
  }

  /// Swapchain creation and configuration
  VkSwapchainObject create_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface) {
    VkSwapchainObject swapchain = {};

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    swapchain.image_extent = surface_capabilities.currentExtent;

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data());

    // NOTE: Device has no preferred format if Vulkan return VK_FORMAT_UNDEFINED
    if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
      Log::info("Swapchain has no preferred format! Nice!");
    } else {
      bool ok = false;
      for (const auto &available_format : surface_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM) {
          Log::info("Swapchain config. found VK_FORMAT_B8G8R8A8_UNORM");
          ok = true;
          break;
        }
      }
      if (!ok) {
        Log::error("Swapchain config. failed.");
        swapchain.handle = VK_NULL_HANDLE;
        return swapchain;
      }
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data());

    if (present_modes.size() >= 1 && surface_formats.size() >= 1) {
      Log::info("Swapchain config okay!");
    } else {
      Log::error("Swapchain config NOT okay!");
    }

    // FIXME: Assumes that the surface format is supported
    VkSurfaceFormatKHR surface_format;
    surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
    swapchain.image_format = surface_format.format;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t swapimages_count = surface_capabilities.minImageCount + 1;
    Log::info("Using swapchain images: " + std::to_string(swapimages_count));
    Log::info("Max swapchain images: " + std::to_string(surface_capabilities.maxImageCount));

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = swapimages_count; // TODO: Same as swapchain_image_count?
    swapchain_ci.imageFormat = surface_format.format;
    swapchain_ci.imageColorSpace = surface_format.colorSpace;
    swapchain_ci.imageExtent = surface_capabilities.currentExtent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // NOTE: Assuming graphics queue == presentation queue
    swapchain_ci.queueFamilyIndexCount = 0;     // Optional
    swapchain_ci.pQueueFamilyIndices = nullptr; // Optional

    // NOTE: Transform applied to surface (Ex: during tablet rotation)
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
      swapchain_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
      swapchain_ci.preTransform = surface_capabilities.currentTransform;
    }

    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_ci, nullptr, &swapchain.handle));

    // Swapchain image and image views config.
    {
      uint32_t swapchain_images_count = 0;
      vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchain_images_count, nullptr);
      swapchain.images.resize(swapchain_images_count);
      vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchain_images_count, swapchain.images.data());

      swapchain.image_views.resize(swapchain_images_count);
      for (size_t i = 0; i < swapchain.images.size(); i++) {
        swapchain.image_views[i] = vk::utils::create_image_view(device, swapchain.images[i], VK_FORMAT_B8G8R8A8_UNORM);
      }
    }

    return swapchain;
   }
} // namespace vk::utils
