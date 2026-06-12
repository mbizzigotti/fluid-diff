#ifdef NDEBUG
#undef NDEBUG
#endif
#define RGFW_VULKAN
#define RGFW_IMPLEMENTATION
#include "3rdparty/RGFW.h"

#define TVK_INSTANCE_EXTENSIONS    \
    VK_KHR_SURFACE_EXTENSION_NAME, \
    RGFW_VK_SURFACE
#define TVK_DEVICE_EXTENSIONS                     \
    VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,    \
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, \
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shader_atomic_float_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT,
    .shaderBufferFloat32Atomics   = VK_TRUE,
    .shaderBufferFloat32AtomicAdd = VK_TRUE,
};
#define TVK_DEVICE_FEATURE_LIST &shader_atomic_float_features
#define TINY_VK_IMPLEMENTATION
#include "3rdparty/tinyvk.h"

#define ensure(E) if (!(E)) printf("error: %s not satified\nlocation: %s:%i\n", #E, __FILE__, __LINE__), exit(1)

TvkContext context;
RGFW_window *window;
TvkSwapChain swap_chain;
VkSurfaceKHR surface;

/* Records an image layout transition into the command buffer. */
static void transition_image(VkCommandBuffer cmd, VkImage image,
                             VkImageLayout old_layout, VkImageLayout new_layout,
                             VkAccessFlags src_access, VkAccessFlags dst_access,
                             VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, 0, 0, 0, 1, &barrier);
}

int main(void)
{
    VkResult result = VK_SUCCESS;
    ensure(tvkCreateContext(TVK_CREATE_CONTEXT_ENABLE_VALIDATION, &context) == VK_SUCCESS);
    ensure((window = RGFW_createWindow("window", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize)) != NULL);
    ensure(RGFW_window_createSurface_Vulkan(window, context.instance, &surface) == VK_SUCCESS);
    ensure(tvkCreateSwapChain(&context, surface, &swap_chain) == VK_SUCCESS);
    
    VkExtent2D extent = swap_chain.extent;

    /* --- Offscreen storage image the compute shader renders into --- */
    VkImage        storage_image;
    VkDeviceMemory storage_memory;
    VkImageView    storage_view;
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { extent.width, extent.height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ensure(tvkCreateImage(&context, &image_info, TVK_MEMORY_USAGE_DEVICE, &storage_image, &storage_memory) == VK_SUCCESS);
    ensure(tvkCreateImageView(context.device, &image_info, storage_image, &storage_view) == VK_SUCCESS);

    /* --- Descriptor set describing the storage image binding --- */
    TvkDescriptorSetBuilder builder = {};
    tvkSetBuilderAppend(&builder, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);
    ensure(tvkSetBuilderCreate(context.device, VK_SHADER_STAGE_COMPUTE_BIT, 1, &builder) == VK_SUCCESS);

    VkDescriptorSet descriptor_set;
    ensure(tvkSetBuilderAllocate(context.device, &builder, &descriptor_set) == VK_SUCCESS);
    VkWriteDescriptorSet write = tvkSetBuilderWrite(&builder, descriptor_set, 0, storage_view);
    vkUpdateDescriptorSets(context.device, 1, &write, 0, 0);

    /* --- Compute pipeline --- */
    VkShaderModule shader;
    ensure(tvkCreateShaderFromFile(context.device, "Render.spv", &shader) == VK_SUCCESS);

    VkShaderModule apply_control_shader;
    ensure(tvkCreateShaderFromFile(context.device, "ApplyControl.spv", &apply_control_shader) == VK_SUCCESS);

    VkPipelineLayout pipeline_layout;
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &builder.layout,
    };
    ensure(vkCreatePipelineLayout(context.device, &layout_info, 0, &pipeline_layout) == VK_SUCCESS);

    VkPipeline pipeline;
    ensure(tvkCreateComputePipeline(context.device, pipeline_layout, shader, &pipeline) == VK_SUCCESS);

    VkPipeline apply_control_pipeline;
    ensure(tvkCreateComputePipeline(context.device, pipeline_layout, apply_control_shader, &apply_control_pipeline) == VK_SUCCESS);

    /* --- Per-frame command buffer and sync objects --- */
    VkCommandBuffer cmd;
    ensure(tvkAllocateCommandBuffers(context.device, context.command_pool, 1, &cmd) == VK_SUCCESS);

    /* One render-finished semaphore per swapchain image (it is waited on by
       present, which only completes once the image is re-acquired), and a
       ring of acquire semaphores so we never reuse one still in flight. */
    VkSemaphore image_available[TVK_MAX_SWAP_CHAIN_IMAGES];
    VkSemaphore render_finished[TVK_MAX_SWAP_CHAIN_IMAGES];
    VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for (uint32_t i = 0; i < swap_chain.image_count; ++i) {
        ensure(vkCreateSemaphore(context.device, &semaphore_info, 0, &image_available[i]) == VK_SUCCESS);
        ensure(vkCreateSemaphore(context.device, &semaphore_info, 0, &render_finished[i]) == VK_SUCCESS);
    }
    VkFence in_flight = tvkCreateFence(context.device, VK_FENCE_CREATE_SIGNALED_BIT);

    uint32_t frame = 0;
    while (RGFW_window_shouldClose(window) == RGFW_FALSE)
    {
        RGFW_event event;
        while (RGFW_window_checkEvent(window, &event));

        vkWaitForFences(context.device, 1, &in_flight, VK_TRUE, UINT64_MAX);
        vkResetFences(context.device, 1, &in_flight);

        VkSemaphore acquire_semaphore = image_available[frame % swap_chain.image_count];
        uint32_t image_index = 0;
        ensure(vkAcquireNextImageKHR(context.device, swap_chain.swap_chain, UINT64_MAX,
                                     acquire_semaphore, 0, &image_index) == VK_SUCCESS);
        VkImage swap_image = swap_chain.images[image_index];

        vkResetCommandBuffer(cmd, 0);
        tvkBeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        /* Prepare the storage image for compute writes. */
        transition_image(cmd, storage_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            0, VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout,
                                0, 1, &descriptor_set, 0, 0);
        vkCmdDispatch(cmd, tvkCeilDiv(extent.width, 16), tvkCeilDiv(extent.height, 16), 1);

        /* Storage image -> transfer source, swapchain image -> transfer dest. */
        transition_image(cmd, storage_image,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        transition_image(cmd, swap_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        /* Blit handles the R8G8B8A8 -> B8G8R8A8 channel swizzle. */
        VkImageBlit blit = {
            .srcSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
            .srcOffsets = { {0, 0, 0}, {(int32_t)extent.width, (int32_t)extent.height, 1} },
            .dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
            .dstOffsets = { {0, 0, 0}, {(int32_t)extent.width, (int32_t)extent.height, 1} },
        };
        vkCmdBlitImage(cmd,
            storage_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swap_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_NEAREST);

        /* Swapchain image -> present. */
        transition_image(cmd, swap_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_ACCESS_TRANSFER_WRITE_BIT, 0,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        vkEndCommandBuffer(cmd);

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        VkSubmitInfo submit = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &acquire_semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_finished[image_index],
        };
        ensure(vkQueueSubmit(context.queue, 1, &submit, in_flight) == VK_SUCCESS);

        VkPresentInfoKHR present = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished[image_index],
            .swapchainCount = 1,
            .pSwapchains = &swap_chain.swap_chain,
            .pImageIndices = &image_index,
        };
        ensure(vkQueuePresentKHR(context.present_queue, &present) == VK_SUCCESS);
        ++frame;
    }

    vkDeviceWaitIdle(context.device);
    RGFW_window_close(window);
}
