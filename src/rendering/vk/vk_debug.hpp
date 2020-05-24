#pragma once

#include <vulkan/vulkan.h>
#include <iostream>

#include "logging.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* userData) {
  Log::warn("Debug callback: " + std::string(pCallbackData->pMessage));
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    Log::error("Debug callback: " + std::string(pCallbackData->pMessage));
  }
  return VK_FALSE;
}
