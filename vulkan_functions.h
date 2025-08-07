// vulkan_functions.h
// This header is used to declare all the Vulkan functions we will load dynamically.
// It's included multiple times with different macro definitions to avoid repetition.

#ifndef EXPORTED_VULKAN_FUNCTION
#define EXPORTED_VULKAN_FUNCTION( name )
#endif

#ifndef GLOBAL_LEVEL_VULKAN_FUNCTION
#define GLOBAL_LEVEL_VULKAN_FUNCTION( name )
#endif

#ifndef INSTANCE_LEVEL_VULKAN_FUNCTION
#define INSTANCE_LEVEL_VULKAN_FUNCTION( name )
#endif

#ifndef DEVICE_LEVEL_VULKAN_FUNCTION
#define DEVICE_LEVEL_VULKAN_FUNCTION( name )
#endif

// Dynamically loaded from libvulkan.so
EXPORTED_VULKAN_FUNCTION( vkGetInstanceProcAddr )
EXPORTED_VULKAN_FUNCTION( vkCreateInstance )

// Global-level functions
GLOBAL_LEVEL_VULKAN_FUNCTION( vkEnumerateInstanceExtensionProperties )
GLOBAL_LEVEL_VULKAN_FUNCTION( vkEnumerateInstanceLayerProperties )

// Instance-level functions
INSTANCE_LEVEL_VULKAN_FUNCTION( vkDestroyInstance )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkEnumeratePhysicalDevices )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkGetPhysicalDeviceProperties )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkGetPhysicalDeviceProperties2 )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkGetPhysicalDeviceQueueFamilyProperties )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkCreateDevice )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkGetDeviceProcAddr )
INSTANCE_LEVEL_VULKAN_FUNCTION( vkGetPhysicalDeviceMemoryProperties )

// Device-level functions
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyDevice )
DEVICE_LEVEL_VULKAN_FUNCTION( vkGetDeviceQueue )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateImage )
DEVICE_LEVEL_VULKAN_FUNCTION( vkGetImageMemoryRequirements )
DEVICE_LEVEL_VULKAN_FUNCTION( vkAllocateMemory )
DEVICE_LEVEL_VULKAN_FUNCTION( vkBindImageMemory )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateImageView )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateRenderPass )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateFramebuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateShaderModule )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreatePipelineLayout )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateGraphicsPipelines )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateCommandPool )
DEVICE_LEVEL_VULKAN_FUNCTION( vkAllocateCommandBuffers )
DEVICE_LEVEL_VULKAN_FUNCTION( vkBeginCommandBuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCmdBeginRenderPass )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCmdBindPipeline )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCmdDraw )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCmdEndRenderPass )
DEVICE_LEVEL_VULKAN_FUNCTION( vkEndCommandBuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkQueueSubmit )
DEVICE_LEVEL_VULKAN_FUNCTION( vkQueueWaitIdle )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCreateBuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkGetBufferMemoryRequirements )
DEVICE_LEVEL_VULKAN_FUNCTION( vkBindBufferMemory )
DEVICE_LEVEL_VULKAN_FUNCTION( vkResetCommandBuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkCmdCopyImageToBuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkMapMemory )
DEVICE_LEVEL_VULKAN_FUNCTION( vkUnmapMemory )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyBuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkFreeMemory )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyShaderModule )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyPipeline )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyPipelineLayout )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyRenderPass )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyFramebuffer )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyImageView )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyImage )
DEVICE_LEVEL_VULKAN_FUNCTION( vkDestroyCommandPool )

#undef EXPORTED_VULKAN_FUNCTION
#undef GLOBAL_LEVEL_VULKAN_FUNCTION
#undef INSTANCE_LEVEL_VULKAN_FUNCTION
#undef DEVICE_LEVEL_VULKAN_FUNCTION
