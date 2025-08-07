// main.c
// A self-contained C program for off-screen rendering with Vulkan.
// It renders a 256x256 image and saves it as 'render.ppm'.

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <dlfcn.h>
#else
#define UNICODE
#include <windows.h>
#endif

// --- Constants ---
const int WIDTH = 256;
const int HEIGHT = 256;
// const char* OUTPUT_FILENAME = "render.ppm";

// --- Vulkan Function Loading ---
// We load function pointers manually.
#define EXPORTED_VULKAN_FUNCTION( name ) PFN_##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION( name ) PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION( name ) PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION( name ) PFN_##name name;

#include "vulkan_functions.h"

// --- Helper Functions ---

void queryAndReportSubgroupSize(VkPhysicalDevice physicalDevice) {
#if defined(VK_KHR_get_physical_device_properties2)
    VkPhysicalDeviceSubgroupProperties subgroupProps = {0};
    subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

    VkPhysicalDeviceProperties2 deviceProps2 = {0};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &subgroupProps;

    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

    // subgroupProps.subgroupSize now holds the subgroup size (number of invocations per subgroup)
    printf("Subgroup size: %u\n", subgroupProps.subgroupSize);
#else
    printf("VK_KHR_get_physical_device_properties2 not supported. Cannot query subgroup size.\n");
#endif
};

// Finds a suitable memory type index.
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

// Reads a SPIR-V shader file.
char* readShaderFile(const char* filename, size_t* pSize) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *pSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(*pSize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate buffer for shader file.\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, *pSize, file);
    fclose(file);
    return buffer;
}

// Creates a VkShaderModule from SPIR-V code.
VkShaderModule createShaderModule(VkDevice device, const char* code, size_t size) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = (const uint32_t*)code;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module!\n");
        exit(EXIT_FAILURE);
    }
    return shaderModule;
}


// --- Main Application Logic ---
int main(int argc, char **argv) {
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

    // --- 2. Create Vulkan Instance ---
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Offscreen Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    VkInstance instance;
    if (vkCreateInstance(&instanceCreateInfo, NULL, &instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance!\n");
        return EXIT_FAILURE;
    }
    
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


    // --- 3. Select Physical Device ---
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "Failed to find GPUs with Vulkan support!\n");
        return EXIT_FAILURE;
    }
    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = -1;

    for (uint32_t i = 0; i < deviceCount; i++) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL);
        VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

        for (uint32_t j = 0; j < queueFamilyCount; j++) {
            if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                physicalDevice = devices[i];
                queueFamilyIndex = j;
                break;
            }
        }
        free(queueFamilies);
        if (physicalDevice != VK_NULL_HANDLE) {
            break;
        }
    }
    free(devices);

    if (physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find a suitable GPU!\n");
        return EXIT_FAILURE;
    }

    // aside: query subgroup size
    queryAndReportSubgroupSize(physicalDevice);

    // --- 4. Create Logical Device and Queue ---
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    
    VkDevice device;
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device!\n");
        return EXIT_FAILURE;
    }

    // Load device level functions
    #define DEVICE_LEVEL_VULKAN_FUNCTION( name ) \
        name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
        if( name == NULL ) { \
            fprintf(stderr, "Could not load device-level Vulkan function %s\n", #name); \
            return EXIT_FAILURE; \
        }
    #include "vulkan_functions.h"

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue);

    // --- 5. Create Offscreen Framebuffer Resources ---
    
    // Color Attachment (Image)
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = WIDTH;
    imageInfo.extent.height = HEIGHT;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, NULL, &colorImage) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create color attachment image!\n");
        return EXIT_FAILURE;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, colorImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, NULL, &colorImageMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate image memory!\n");
        return EXIT_FAILURE;
    }
    vkBindImageMemory(device, colorImage, colorImageMemory, 0);

    // Image View
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = colorImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, NULL, &colorImageView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image view!\n");
        return EXIT_FAILURE;
    }

    // Render Pass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass!\n");
        return EXIT_FAILURE;
    }

    // Framebuffer
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &colorImageView;
    framebufferInfo.width = WIDTH;
    framebufferInfo.height = HEIGHT;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create framebuffer!\n");
        return EXIT_FAILURE;
    }

    // --- 6. Create Graphics Pipeline ---
    size_t vertShaderSize, fragShaderSize;
    char* vertShaderCode = readShaderFile(argv[1], &vertShaderSize);
    char* fragShaderCode = readShaderFile(argv[2], &fragShaderSize);
    
    if (!vertShaderCode || !fragShaderCode) {
        return EXIT_FAILURE;
    }

    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode, vertShaderSize);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode, fragShaderSize);

    free(vertShaderCode);
    free(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)WIDTH;
    viewport.height = (float)HEIGHT;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = WIDTH;
    scissor.extent.height = HEIGHT;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout!\n");
        return EXIT_FAILURE;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline!\n");
        return EXIT_FAILURE;
    }

    vkDestroyShaderModule(device, fragShaderModule, NULL);
    vkDestroyShaderModule(device, vertShaderModule, NULL);

    // --- 7. Create Command Pool and Command Buffer ---
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    
    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool!\n");
        return EXIT_FAILURE;
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);

    // --- 8. Record Drawing Commands ---
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = WIDTH;
    renderPassBeginInfo.renderArea.extent.height = HEIGHT;
    
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Draw a single triangle
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    // --- 9. Submit Commands and Wait ---
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    // --- 10. Copy Image to Buffer and Save to File ---
    // Create a host-visible buffer to copy the image data into
    VkBuffer dstBuffer;
    VkDeviceMemory dstBufferMemory;
    VkDeviceSize bufferSize = WIDTH * HEIGHT * 4; // 4 bytes per pixel (R8G8B8A8)

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &dstBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create destination buffer!\n");
        return EXIT_FAILURE;
    }

    vkGetBufferMemoryRequirements(device, dstBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, NULL, &dstBufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate destination buffer memory!\n");
        return EXIT_FAILURE;
    }
    vkBindBufferMemory(device, dstBuffer, dstBufferMemory, 0);

    // Record copy command
    vkResetCommandBuffer(commandBuffer, 0);
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){0, 0, 0};
    region.imageExtent = (VkExtent3D){WIDTH, HEIGHT, 1};

    vkCmdCopyImageToBuffer(commandBuffer, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer, 1, &region);
    
    vkEndCommandBuffer(commandBuffer);

    // Submit copy command
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    // Map memory and save to file
    void* data;
    vkMapMemory(device, dstBufferMemory, 0, bufferSize, 0, &data);

    FILE* file = fopen(argv[3], "wb");
    if (!file) {
        fprintf(stderr, "Failed to open output file!\n");
    } else {
        fprintf(file, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                // PPM expects RGB, our buffer is RGBA. We skip the alpha channel.
                fwrite((unsigned char*)data + (y * WIDTH + x) * 4, 3, 1, file);
            }
        }
        fclose(file);
        printf("Successfully rendered image to %s\n", argv[3]);
    }
    
    vkUnmapMemory(device, dstBufferMemory);

    // --- 11. Cleanup ---
    vkDestroyBuffer(device, dstBuffer, NULL);
    vkFreeMemory(device, dstBufferMemory, NULL);
    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    vkDestroyFramebuffer(device, framebuffer, NULL);
    vkDestroyImageView(device, colorImageView, NULL);
    vkDestroyImage(device, colorImage, NULL);
    vkFreeMemory(device, colorImageMemory, NULL);
    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);

#if defined(__linux__)
    dlclose(vulkan_library);
#elif defined(_WIN32)
    FreeLibrary(vulkan_library);
#endif

    return EXIT_SUCCESS;
}
