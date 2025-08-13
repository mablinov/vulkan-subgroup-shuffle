#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __linux__
#include <dlfcn.h>
#else
#define UNICODE
#include <windows.h>
#endif

// Define the dimensions of the image we want to generate.
#define IMAGE_WIDTH 256
#define IMAGE_HEIGHT 256

#define EXPORTED_VULKAN_FUNCTION( name ) PFN_##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION( name ) PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION( name ) PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION( name ) PFN_##name name;

#include "vulkan_functions.h"

// Simple error handling macro.
#define VK_CHECK(result)                                                 \
    if (result != VK_SUCCESS) {                                          \
        fprintf(stderr, "Vulkan error in %s at line %d: %d\n", __FILE__, \
                __LINE__, result);                                       \
        exit(EXIT_FAILURE);                                              \
    }

// Function to find a suitable memory type index.
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    fprintf(stderr, "Failed to find suitable memory type!\n");
    exit(EXIT_FAILURE);
}

// Function to read a binary file (like our SPIR-V shader).
char* readFile(const char* filename, size_t* pSize) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    *pSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(*pSize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file content.\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, *pSize, file);
    fclose(file);
    return buffer;
}

// Function to save the image buffer to a .ppm file.
void saveImage(const char* filename, const void* buffer, uint32_t width, uint32_t height) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filename);
        return;
    }

    // Write PPM header (P6 format for binary color).
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // The shader outputs RGBA, but PPM P6 is RGB. We need to convert.
    const uint8_t* pixelData = (const uint8_t*)buffer;
    for (uint32_t i = 0; i < width * height; ++i) {
        fwrite(pixelData, 3, 1, file); // Write R, G, B
        pixelData += 4;                 // Skip Alpha
    }

    fclose(file);
    printf("Image saved to %s\n", filename);
}


int main() {
#if defined(__linux__)
    // --- 1. Load Vulkan Loader ---
    void* vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!vulkan_library) {
        fprintf(stderr, "Failed to load Vulkan library.\n");
        return EXIT_FAILURE;
    }
    
    // Load exported functions
    #define EXPORTED_VULKAN_FUNCTION( name ) \
        name = (PFN_##name)dlsym(vulkan_library, #name); \
        if( name == NULL ) { \
            fprintf(stderr, "Could not load exported Vulkan function %s\n", #name); \
            return EXIT_FAILURE; \
        }
#elif defined(_WIN32)
    HMODULE vulkan_library = LoadLibrary(L"vulkan-1.dll");
    if (!vulkan_library) {
        fprintf(stderr, "Failed to load vulkan-1.dll.\n");
        return EXIT_FAILURE;
    }

    // Load exported functions
    #define EXPORTED_VULKAN_FUNCTION( name ) \
        name = (PFN_##name)GetProcAddress(vulkan_library, #name); \
        if( name == NULL ) { \
            fprintf(stderr, "Could not load exported Vulkan function %s\n", #name); \
            return EXIT_FAILURE; \
        }
#else
    #error "Unsupported platform"
#endif

    #include "vulkan_functions.h"

    // --- 1. Vulkan Instance and Device Setup ---

    // Create a Vulkan instance.
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Compute",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_1,
    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };
    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instanceCreateInfo, NULL, &instance));

    // Load global and instance level functions
    #define GLOBAL_LEVEL_VULKAN_FUNCTION( name ) \
        name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name); \
        if( name == NULL ) { \
            fprintf(stderr, "Could not load global-level Vulkan function %s\n", #name); \
            return EXIT_FAILURE; \
        }
    #define INSTANCE_LEVEL_VULKAN_FUNCTION( name ) \
        name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
        if( name == NULL ) { \
            fprintf(stderr, "Could not load instance-level Vulkan function %s\n", #name); \
            return EXIT_FAILURE; \
        }
    #include "vulkan_functions.h"

    // Select a physical device.
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
    VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t computeQueueFamilyIndex = UINT32_MAX;

    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, NULL);
        VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilies);

        for (uint32_t j = 0; j < queueFamilyCount; j++) {
            if (queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                physicalDevice = physicalDevices[i];
                computeQueueFamilyIndex = j;
                break;
            }
        }
        free(queueFamilies);
        if (physicalDevice != VK_NULL_HANDLE) break;
    }
    free(physicalDevices);

    if (physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find a suitable physical device with a compute queue.\n");
        return EXIT_FAILURE;
    }

    // Create a logical device.
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = computeQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
    };
    VkDevice device;
    VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device));

    // Load device level functions
    #define DEVICE_LEVEL_VULKAN_FUNCTION( name ) \
        name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
        if( name == NULL ) { \
            fprintf(stderr, "Could not load device-level Vulkan function %s\n", #name); \
            return EXIT_FAILURE; \
        }
    #include "vulkan_functions.h"

    // Get the compute queue.
    VkQueue computeQueue;
    vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);

    // --- 2. Create Storage Image and Buffer ---

    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM, // RGBA 8-bit unsigned normalized
        .extent = {IMAGE_WIDTH, IMAGE_HEIGHT, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkImage image;
    VK_CHECK(vkCreateImage(device, &imageCreateInfo, NULL, &image));

    // Allocate memory for the image.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    VkDeviceMemory imageMemory;
    VK_CHECK(vkAllocateMemory(device, &allocInfo, NULL, &imageMemory));
    VK_CHECK(vkBindImageMemory(device, image, imageMemory, 0));

    // Create an image view.
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    VkImageView imageView;
    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, NULL, &imageView));

    // Create a buffer to copy the image data to for reading on the CPU.
    VkDeviceSize bufferSize = IMAGE_WIDTH * IMAGE_HEIGHT * 4; // 4 bytes per pixel (RGBA)
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer stagingBuffer;
    VK_CHECK(vkCreateBuffer(device, &bufferCreateInfo, NULL, &stagingBuffer));

    // Allocate memory for the staging buffer.
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkDeviceMemory stagingBufferMemory;
    VK_CHECK(vkAllocateMemory(device, &allocInfo, NULL, &stagingBufferMemory));
    VK_CHECK(vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0));


    // --- 3. Descriptor Set and Pipeline Setup ---

    // Create a descriptor set layout.
    VkDescriptorSetLayoutBinding layoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &layoutBinding,
    };
    VkDescriptorSetLayout descriptorSetLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, NULL, &descriptorSetLayout));

    // Create a descriptor pool.
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
    };
    VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
        .maxSets = 1,
    };
    VkDescriptorPool descriptorPool;
    VK_CHECK(vkCreateDescriptorPool(device, &poolCreateInfo, NULL, &descriptorPool));

    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    VkDescriptorSet descriptorSet;
    VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

    // Update the descriptor set to point to our image view.
    VkDescriptorImageInfo imageInfo = {
        .imageView = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet writeDescriptorSet = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .pImageInfo = &imageInfo,
    };
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
    
    // Create the compute pipeline.
    size_t shaderCodeSize;
    char* shaderCode = readFile("spv/shaderComputeSubgroupShuffle.comp.spv", &shaderCodeSize);
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCodeSize,
        .pCode = (const uint32_t*)shaderCode,
    };
    VkShaderModule computeShaderModule;
    VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &computeShaderModule));
    free(shaderCode);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &pipelineLayout));

    VkComputePipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeShaderModule,
            .pName = "main",
        },
        .layout = pipelineLayout,
    };
    VkPipeline pipeline;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline));

    // --- 4. Command Buffer Recording and Submission ---

    // Create a command pool and command buffer.
    VkCommandPoolCreateInfo cmdPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = computeQueueFamilyIndex,
    };
    VkCommandPool commandPool;
    VK_CHECK(vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &commandPool));

    VkCommandBufferAllocateInfo cmdBufAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(device, &cmdBufAllocInfo, &commandBuffer));

    // Record commands.
    VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    // Transition image layout to general for shader writing.
    VkImageMemoryBarrier barrier1 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier1);

    // Bind pipeline and descriptor sets.
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

    // Dispatch the compute shader.
    // We need to dispatch enough workgroups to cover the entire image.
    // Workgroup size is 16x16, image is 256x256. So, 256/16 = 16 workgroups in each dimension.
    vkCmdDispatch(commandBuffer, IMAGE_WIDTH / 16, IMAGE_HEIGHT / 16, 1);

    // Transition image layout for transfer source.
    VkImageMemoryBarrier barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier2);

    // Copy image to staging buffer.
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {IMAGE_WIDTH, IMAGE_HEIGHT, 1},
    };
    vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    // Submit to the queue and wait for completion.
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    VkFenceCreateInfo fenceCreateInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, NULL, &fence));

    VK_CHECK(vkQueueSubmit(computeQueue, 1, &submitInfo, fence));
    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

    // --- 5. Read Data and Cleanup ---

    // Map memory, read data, and save to file.
    void* mappedMemory = NULL;
    VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &mappedMemory));
    saveImage("output.ppm", mappedMemory, IMAGE_WIDTH, IMAGE_HEIGHT);
    vkUnmapMemory(device, stagingBufferMemory);

    // Cleanup Vulkan objects.
    vkDestroyFence(device, fence, NULL);
    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyShaderModule(device, computeShaderModule, NULL);
    vkDestroyDescriptorPool(device, descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
    vkDestroyImageView(device, imageView, NULL);
    vkDestroyImage(device, image, NULL);
    vkFreeMemory(device, imageMemory, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);

    return EXIT_SUCCESS;
}
