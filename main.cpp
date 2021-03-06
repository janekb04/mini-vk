#include <mimalloc-new-delete.h>  // override all allocations with the optimized mimalloc allocator library; NOTE: consider calling mi_option_set for some performance tweaks

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1  // load Vulkan dynamically rather than statically
#define VULKAN_HPP_NO_CONSTRUCTORS            // use C++20's designated initializers
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS     // as above; NOTE: for compatibility
                                              // with older SDKs
#define VULKAN_HPP_NO_SETTERS                 // disable setter methods as unneeded and
                                              // unnecessarily extending compilation time
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR     // use C++20's spaceship operator;
                                              // NOTE: for compatibility with older
                                              // SDKs
#include <vulkan/vulkan.hpp>  // use the C++ bindings for Vulkan instead of the C headers; NOTE: there is also a higher level wrapper called vulkan_raii.hpp with a different interface
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE  // has to be defined exactly
                                                    // once when using
                                                    // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC

#include <glfwpp/glfwpp.h>  // use my (janekb04 at Github) C++ wrapper for GLFW

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>

    namespace ranges = std::ranges;

// Shortcuts
#define FWD(...) /* perfect ForWarDing */ ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define XPL(...) /* eXPression Lambda */                                                                             \
    [&](auto&&... _args) {                                                                                           \
        auto __arg_holder = std::tuple{FWD(_args)..., nullptr};                                                      \
        [[maybe_unused]] auto&& _0 = std::get<std::min(static_cast<size_t>(0), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _1 = std::get<std::min(static_cast<size_t>(1), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _2 = std::get<std::min(static_cast<size_t>(2), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _3 = std::get<std::min(static_cast<size_t>(3), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _4 = std::get<std::min(static_cast<size_t>(4), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _5 = std::get<std::min(static_cast<size_t>(5), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _6 = std::get<std::min(static_cast<size_t>(6), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _7 = std::get<std::min(static_cast<size_t>(7), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _8 = std::get<std::min(static_cast<size_t>(8), sizeof...(_args) - 1)>(__arg_holder); \
        [[maybe_unused]] auto&& _9 = std::get<std::min(static_cast<size_t>(9), sizeof...(_args) - 1)>(__arg_holder); \
        return __VA_ARGS__;                                                                                          \
    }

const int WND_WIDTH = 800,
          WND_HEIGHT = 600;  // window dimensions (for now non-resizable)
const char* const APP_NAME = "vk_mini";
const uint32_t APP_API_VERSION = VK_MAKE_API_VERSION(0, 1, 2, 0);
const auto APP_LAYERS =
#ifdef NDEBUG
    std::array<const char*, 0>{};
#else
    std::array{"VK_LAYER_KHRONOS_validation"};  // contrary to extensions there is no VK_KHRONOS_VALIDATION_LAYER_NAME
#endif

const std::array APP_DEVICE_EXTENSIONS{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const vk::AllocationCallbacks APP_ALLOCATION_CALLBACKS{
    // NOTE: consider not using allocation callbacks as the performance
    // gain/loss hasn't been measured. They are intended for logging rather than
    // for a performance gain.
    .pUserData = nullptr,
    .pfnAllocation =
        [](void* /*pUserData*/, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/) {
            return mi_malloc_aligned(size, alignment);
        },
    .pfnReallocation =
        [](void* /*pUserData*/, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/) {
            return mi_realloc_aligned(pOriginal, size, alignment);
        },
    .pfnFree = [](void* /*pUserData*/, void* pMemory) { mi_free(pMemory); }
    // NOTE: callbacks can be installed for internal allocations. They are only
    // callbacks that will notify the application that the Vulkan implementation
    // performed an allocation using its own mechanisms.
};
const char* const APP_VERTEX_SHADER_PATH = "basic.vert.spv";
const char* const APP_VERTEX_SHADER_ENTRY_POINT = "main";
const char* const APP_FRAGMENT_SHADER_PATH = "basic.frag.spv";
const char* const APP_FRAGMENT_SHADER_ENTRY_POINT = "main";
const auto APP_SAMPLE_COUNT = vk::SampleCountFlagBits::e1;
const uint32_t APP_GRAPHICS_PIPELINE_SUBPASS_INDEX = 0;
const auto APP_SUBPASS_PIPELINE_BIND_POINT = vk::PipelineBindPoint::eGraphics;

[[nodiscard]] auto read_binary_file(const std::filesystem::path& p) {
    std::ifstream in{p, std::ios_base::in | std::ios_base::binary};
    if (!in.is_open()) {
        throw std::runtime_error("Couldn't open file " + p.string());
    }
    size_t sz = std::filesystem::file_size(p);  // can't throw as `p` exists
    auto buffer = std::make_unique_for_overwrite<std::byte[]>(sz);
    in.read(reinterpret_cast<char*>(buffer.get()), sz);
    return std::tuple{std::move(buffer), sz};
}

int main() {
    try {
        // Initialize GLFW and create window
        auto GLFW = glfw::init();
        glfw::WindowHints{.resizable = false, .clientApi = glfw::ClientApi::None}.apply();
        glfw::Window window{WND_WIDTH, WND_HEIGHT, APP_NAME};

        // Load global Vulkan functions
        vk::DynamicLoader dl;  // has destructor
        {
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
                dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }

        // Create Vulkan instance
        auto instance = []() {
            uint32_t implementation_api_version =
                VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion ? vk::enumerateInstanceVersion() : VK_API_VERSION_1_0;

            if (implementation_api_version < APP_API_VERSION) {
                throw std::runtime_error(
                    "The Vulkan implementation doesn't support the version required by "
                    "the application");
            }

            vk::ApplicationInfo appInfo{.pApplicationName = APP_NAME,
                                        .applicationVersion = 1,
                                        .pEngineName = APP_NAME,
                                        .engineVersion = 1,
                                        .apiVersion = APP_API_VERSION};

            auto instanceExtensions = glfw::getRequiredInstanceExtensions();

            return vk::createInstance(
                vk::InstanceCreateInfo{// NOTE: use VkDebugUtilsMessengerEXT pNext to debug instance
                                       // creation and destruction
                                       .pApplicationInfo = &appInfo,
                                       .enabledLayerCount = APP_LAYERS.size(),
                                       .ppEnabledLayerNames = APP_LAYERS.data(),  // NOTE: vk::EnumerateInstanceLayerProperties to
                                                                                  // handle optional layers
                                       .enabledExtensionCount = static_cast<uint32_t>(
                                           instanceExtensions.size()),  // NOTE:
                                                                        // vk::EnumerateInstanceExtensionProperties
                                                                        // to handle optional extensions
                                       .ppEnabledExtensionNames = instanceExtensions.data()},
                APP_ALLOCATION_CALLBACKS  // it is sufficient to pass the allocation
                                          // callbacks only here and at .destroy(),
                                          // they will be used by all child objects of
                                          // this instance as per the documentation of
                                          // VkSystemAllocationScope
            );
        }();

        auto surface = window.createSurface(instance);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);  // load instance-specific function pointers

        // NOTE: create a VkDebugUtilsMessengerEXT for custom error callback instead
        // of default print to stdout

        auto [physicalDeviceGroup, graphicsFamilyIdx, presentFamilyIdx] = [&instance, &surface]() {
            auto physicalDeviceGroups = instance.enumeratePhysicalDeviceGroups();
            for (auto&& physicalDeviceGroup : physicalDeviceGroups) {
                auto physicalDevice = physicalDeviceGroup.physicalDevices[0];  // the group is guaranteed to
                                                                               // have at least one
                                                                               // physical device, while all
                                                                               // physical devices
                                                                               // in the group are required
                                                                               // to have the same features,
                                                                               // extensions and properties,
                                                                               // so only one needs to be examined

                auto [graphicsFamilyIdx, presentFamilyIdx] = [&physicalDevice, &surface]() {
                    auto queueFamilies = physicalDevice.getQueueFamilyProperties2();
                    std::optional<uint32_t> graphicsFamilyIdx, presentFamilyIdx;
                    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
                        if (queueFamilies[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
                            graphicsFamilyIdx = i;
                        }
                        if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
                            presentFamilyIdx = i;
                        }
                    }
                    return std::tuple{graphicsFamilyIdx, presentFamilyIdx};
                }();
                if (!graphicsFamilyIdx.has_value() || !presentFamilyIdx.has_value()) {
                    continue;
                }

                {
                    bool extensionsSupported = [&physicalDevice]() {
                        auto supportedExtensions = physicalDevice.enumerateDeviceExtensionProperties();

                        // NOTE: possibly use a different data structure to find if all extensions are supported
                        for (auto&& requiredExtension : APP_DEVICE_EXTENSIONS) {
                            if (!ranges::any_of(supportedExtensions, XPL(strcmp(requiredExtension, _0.extensionName) == 0))) {
                                return false;
                            }
                        }
                        return true;
                    }();
                    if (!extensionsSupported) {
                        continue;
                    }
                }

                return std::tuple{physicalDeviceGroup, *graphicsFamilyIdx, *presentFamilyIdx};
                // NOTE: physicalDevice.getProperties2, .getFeatures2 and similar to
                // check for some things as currently the first supported GPU is returned, rather than the best
            }
            throw std::runtime_error("No suitable GPU found");
            // NOTE: when a device is deemed not suitable, possibly print an error message
            // indicating why, so the user may know that some GPUs are not supported for informative reasons
        }();

        // Create logical device with graphics queue
        auto [device, graphicsQueue, presentQueue] = [&physicalDeviceGroup, &graphicsFamilyIdx, &presentFamilyIdx]() {
            float graphicsQueuePriority = 1.0f, presentQueuePriority = 1.0f;
            vk::DeviceQueueCreateFlags graphicsQueueFlags{}, presentQueueFlags{};

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{.flags = graphicsQueueFlags,
                                                                 .queueFamilyIndex = graphicsFamilyIdx,
                                                                 .queueCount = 1,
                                                                 .pQueuePriorities = &graphicsQueuePriority});
            if (graphicsFamilyIdx != presentFamilyIdx) {
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{.flags = presentQueueFlags,
                                                                     .queueFamilyIndex = presentFamilyIdx,
                                                                     .queueCount = 1,
                                                                     .pQueuePriorities = &presentQueuePriority});
            }

            vk::StructureChain deviceCreateInfo{
                vk::DeviceCreateInfo{
                    .flags = vk::DeviceCreateFlags{},
                    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
                    .pQueueCreateInfos = queueCreateInfos.data(),
                    .enabledLayerCount = APP_LAYERS.size(),
                    .ppEnabledLayerNames = APP_LAYERS.data(),
                    .enabledExtensionCount = APP_DEVICE_EXTENSIONS.size(),
                    .ppEnabledExtensionNames = APP_DEVICE_EXTENSIONS.data(),
                    .pEnabledFeatures = nullptr  // using PhysicalDeviceFeatures2 instead
                },
                vk::PhysicalDeviceFeatures2{.features = vk::PhysicalDeviceFeatures{}},
                vk::DeviceGroupDeviceCreateInfo{.physicalDeviceCount = physicalDeviceGroup.physicalDeviceCount,
                                                .pPhysicalDevices = physicalDeviceGroup.physicalDevices}};
            auto device = physicalDeviceGroup.physicalDevices[0].createDevice(deviceCreateInfo.get());

            vk::DeviceQueueInfo2 graphicsQueueInfo{
                .flags = graphicsQueueFlags, .queueFamilyIndex = graphicsFamilyIdx, .queueIndex = 0};
            vk::DeviceQueueInfo2 presentQueueInfo{
                .flags = presentQueueFlags, .queueFamilyIndex = presentFamilyIdx, .queueIndex = 0};

            return std::tuple{std::move(device), device.getQueue2(graphicsQueueInfo), device.getQueue2(presentQueueInfo)};
        }();

        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);  // load device-specific function pointers

        auto commandPool = [&device, &graphicsFamilyIdx]() {
            vk::CommandPoolCreateInfo commandPoolCreateInfo{
                .flags{},  // NOTE: often CommandPoolCreateFlagBits::eTransient is used (when recording cmdbufs frequently)
                .queueFamilyIndex = graphicsFamilyIdx};

            return device.createCommandPool(commandPoolCreateInfo);
        }();

        // Create swapchain with images
        auto [swapchain, swapchainImageFormat, swapchainImageExtent, swapchainImages,
              maxFramesInFlight] = [&physicalDeviceGroup, &surface, &graphicsFamilyIdx, &presentFamilyIdx, &window, &device]() {
            // NOTE: possibly use the newer .getSurfaceCapabilities2KHR and similar instead; requires
            // the VK_KHR_get_surface_capabilities2 extension
            auto surfaceFormat = [&physicalDeviceGroup, &surface]() {
                auto surfaceFormats = physicalDeviceGroup.physicalDevices[0].getSurfaceFormatsKHR(surface);
                for (auto&& surfaceFormat : surfaceFormats) {
                    if (surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                        // NOTE: possibly use HDR
                        switch (surfaceFormat.format) {
                            case vk::Format::eR8G8B8A8Srgb:
                            case vk::Format::eB8G8R8A8Srgb:
                                return surfaceFormat;
                        }
                    }
                }
                return surfaceFormats[0];
            }();
            auto presentMode = [&physicalDeviceGroup, &surface]() {
                auto presentModes = physicalDeviceGroup.physicalDevices[0].getSurfacePresentModesKHR(surface);
                for (auto&& presentMode : presentModes) {
                    // NOTE: mailbox mode is not good for power consumption
                    // NOTE: possibly use VK_KHR_shared_presentable_image for better performance
                    if (presentMode == vk::PresentModeKHR::eMailbox) {  // triple (or more) buffering
                        return presentMode;
                    }
                }
                return vk::PresentModeKHR::eFifo;  // guaranteed to be supported
            }();
            auto surfaceCapabilities = physicalDeviceGroup.physicalDevices[0].getSurfaceCapabilitiesKHR(surface);
            auto extent = [&surfaceCapabilities, &window]() {
                if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() ||
                    surfaceCapabilities.currentExtent.height == std::numeric_limits<uint32_t>::max()) {
                    size_t windowFramebufferWidth,
                        windowFramebufferHeight;  // NOTE: for some weird reason MSVC doesn't like a structured binding here
                    std::tie(windowFramebufferWidth, windowFramebufferHeight) = window.getFramebufferSize();
                    return vk::Extent2D{
                        std::clamp(static_cast<uint32_t>(windowFramebufferWidth), surfaceCapabilities.minImageExtent.width,
                                   surfaceCapabilities.maxImageExtent.width),
                        std::clamp(static_cast<uint32_t>(windowFramebufferHeight), surfaceCapabilities.minImageExtent.height,
                                   surfaceCapabilities.maxImageExtent.height)};
                }
                return surfaceCapabilities.currentExtent;
            }();
            uint32_t minOptimalImageCount = 3;  // according to https://github.com/KhronosGroup/Vulkan-Docs/issues/909
            uint32_t imageCount = std::clamp(minOptimalImageCount, surfaceCapabilities.minImageCount,
                                             (surfaceCapabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max()
                                                                                     : surfaceCapabilities.maxImageCount));
            // NOTE: should always use eExclusive and manually synchronize for better performance
            auto imageSharingMode =
                (graphicsFamilyIdx != presentFamilyIdx) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
            auto queueFamilyIndices = ((graphicsFamilyIdx != presentFamilyIdx) ? std::vector{graphicsFamilyIdx, presentFamilyIdx}
                                                                               : std::vector<uint32_t>{});
            auto compositeAlpha = (window.getAttribTransparentFramebuffer() &&
                                   surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
                                      ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
                                      : vk::CompositeAlphaFlagBitsKHR::eOpaque;
            // NOTE: consider using VK_EXT_full_screen_exclusive for potentially better performance
            vk::SwapchainCreateInfoKHR swapchainCreateInfo{
                .pNext{},
                .flags{},
                .surface{surface},
                .minImageCount{imageCount},
                .imageFormat{surfaceFormat.format},
                .imageColorSpace{surfaceFormat.colorSpace},
                .imageExtent{extent},
                .imageArrayLayers{1},                                   // NOTE: >1 for VR
                .imageUsage{vk::ImageUsageFlagBits::eColorAttachment},  // NOTE: different flags can be used for different
                                                                        // purposes; for example if rendering is done in a compute
                                                                        // shader and then the image is copied, or if the image is
                                                                        // to be encoded (ex. when recording footage)
                .imageSharingMode{imageSharingMode},
                .queueFamilyIndexCount{static_cast<uint32_t>(queueFamilyIndices.size())},
                .pQueueFamilyIndices{queueFamilyIndices.data()},
                .preTransform{surfaceCapabilities.currentTransform},  // NOTE: in rare circumstances may be useful to change
                .compositeAlpha{compositeAlpha},
                .presentMode{presentMode},
                .clipped{true},                 // NOTE: set to false if framebuffer should be readable (ex. when recording)
                .oldSwapchain = VK_NULL_HANDLE  // NOTE: set to old swapchain when recreating on resize
            };

            auto swapchain = device.createSwapchainKHR(swapchainCreateInfo);
            auto swapchainImages = device.getSwapchainImagesKHR(swapchain);
            auto maxFramesInFlight = swapchainImages.size() - surfaceCapabilities.minImageCount;
            return std::tuple{std::move(swapchain), surfaceFormat.format, extent, std::move(swapchainImages), maxFramesInFlight};
        }();

        // NOTE: use timeline semaphores and VK_KHR_synchronization2 for synchronization
        auto [currentImageAcquiredSemaphores, imageRenderedSemaphores,
              imageRenderedFences] = [&device, swapChainImageCount = swapchainImages.size()]() {
            std::vector<vk::Semaphore> currentImageAcquiredSemaphores{swapChainImageCount, VK_NULL_HANDLE};
            std::vector<vk::Semaphore> imageRenderedSemaphores;
            std::vector<vk::Fence> imageRenderedFences;
            imageRenderedSemaphores.reserve(swapChainImageCount);
            imageRenderedFences.reserve(swapChainImageCount);

            for (int i = 0; i < swapChainImageCount; ++i) {
                imageRenderedSemaphores.push_back(device.createSemaphore(vk::SemaphoreCreateInfo{}));
                imageRenderedFences.push_back(
                    device.createFence(vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
            }

            return std::tuple{std::move(currentImageAcquiredSemaphores), std::move(imageRenderedSemaphores),
                              std::move(imageRenderedFences)};
        }();

        // NOTE: a single cmdbuf wouldn't suffice as that would make it impossible to have more than one frame in flight as a
        // single command buffer referes to a single framebuffer at a given time. at the same time, though, maybe it could be
        // reset and submited without invalidating the previous (already submited) version of itself?
        auto commandBuffers = [&device, &commandPool, swapChainImageCount = swapchainImages.size()]() {
            vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
                .commandPool = commandPool,
                .level = vk::CommandBufferLevel::ePrimary,  // NOTE: secondary command buffers can be used to be invoked from
                                                            // primary command buffers
                .commandBufferCount =
                    static_cast<uint32_t>(swapChainImageCount)  // one per frambuffer -> one per image view -> one per image
            };
            return device.allocateCommandBuffers(commandBufferAllocateInfo);
        }();

        auto swapchainImageViews = [&device, &swapchainImages, &swapchainImageFormat]() {
            std::vector<vk::ImageView> swapchainImageViews;
            swapchainImageViews.reserve(swapchainImages.size());

            for (auto&& image : swapchainImages) {
                vk::ImageViewCreateInfo imageViewCreateInfo{.image{image},
                                                            .viewType{vk::ImageViewType::e2D},
                                                            .format{swapchainImageFormat},
                                                            .components{.r{vk::ComponentSwizzle::eIdentity},
                                                                        .g{vk::ComponentSwizzle::eIdentity},
                                                                        .b{vk::ComponentSwizzle::eIdentity},
                                                                        .a{vk::ComponentSwizzle::eIdentity}},
                                                            .subresourceRange{
                                                                .aspectMask{vk::ImageAspectFlagBits::eColor},
                                                                .baseMipLevel{0},
                                                                .levelCount{1},
                                                                .baseArrayLayer{0},
                                                                .layerCount{1}  // NOTE: == swapchainCreateInfo.imageArrayLayers
                                                            }};
                swapchainImageViews.push_back(device.createImageView(imageViewCreateInfo));
            }

            return swapchainImageViews;
        }();

        auto renderpass = [&device, &swapchainImageFormat]() {
            auto attachments = {vk::AttachmentDescription2{
                .format = swapchainImageFormat,
                .samples = APP_SAMPLE_COUNT,
                .loadOp = vk::AttachmentLoadOp::eDontCare,  // NOTE: can be used to clear image before rendering
                .storeOp = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,  // NOTE: should be changed when using stencil buffers
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout = vk::ImageLayout::eUndefined,  // NOTE: can only be used in combination with LoadOp::eDontCare
                .finalLayout = vk::ImageLayout::ePresentSrcKHR}};

            vk::AttachmentReference2 mainColorAttachmentReference{
                .attachment = 0,
                .layout = vk::ImageLayout::eColorAttachmentOptimal,
                .aspectMask{}};  // ignored, as it doesn't refer to an input attachment

            auto subpasses = {vk::SubpassDescription2{
                .pipelineBindPoint = APP_SUBPASS_PIPELINE_BIND_POINT,
                .viewMask{},                // NOTE: to be used with multiview
                .inputAttachmentCount = 0,  // NOTE: used to set input attachments
                .pInputAttachments = nullptr,
                .colorAttachmentCount = 1,
                .pColorAttachments = &mainColorAttachmentReference,
                .pResolveAttachments = nullptr,      // NOTE: has something to do with multisampling
                .pDepthStencilAttachment = nullptr,  // NOTE: should be used with depth/stencil buffer
                .preserveAttachmentCount = 0,        // NOTE: used for any attachments that are not accessed in this subpass, but
                                                     // shouldn't have their contents invalidated
                .pPreserveAttachments = 0}};

            auto dependencies = {vk::SubpassDependency2{
                .srcSubpass = VK_SUBPASS_EXTERNAL,  // operations before the first subpass
                .dstSubpass = APP_GRAPHICS_PIPELINE_SUBPASS_INDEX,
                .srcStageMask =
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,  // wait until everyone before us is done with the image
                .dstStageMask =
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,  // we wait until the image is ready to be written to
                .srcAccessMask{},  // NOTE: not sure what this does, but I'll just move on for now and get back to it
                .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                .dependencyFlags{}  // NOTE: also not sure what this does
            }};

            vk::RenderPassCreateInfo2 renderPassCreateInfo{
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = std::data(attachments),
                .subpassCount = static_cast<uint32_t>(subpasses.size()),
                .pSubpasses = std::data(subpasses),
                .dependencyCount = static_cast<uint32_t>(dependencies.size()),
                .pDependencies = std::data(dependencies),
                .correlatedViewMaskCount = 0,  // NOTE: has something to do with multiview
                .pCorrelatedViewMasks = nullptr};

            return device.createRenderPass2(renderPassCreateInfo);
        }();

        auto framebuffers = [&device, &renderpass, &swapchainImageViews, &swapchainImageExtent]() {
            std::vector<vk::Framebuffer> framebuffers;
            framebuffers.reserve(swapchainImageViews.size());

            for (auto&& image : swapchainImageViews) {
                vk::FramebufferCreateInfo framebufferCreateInfo{.renderPass = renderpass,
                                                                .attachmentCount = 1,
                                                                .pAttachments = &image,
                                                                .width = swapchainImageExtent.width,
                                                                .height = swapchainImageExtent.height,
                                                                .layers = 1};
                framebuffers.push_back(device.createFramebuffer(framebufferCreateInfo));
            }

            return framebuffers;
        }();

        auto [graphicsPipeline, graphicsPipelineLayout] = [&device, &swapchainImageExtent, &renderpass]() {
            auto [vertexShaderModule, fragmentShaderModule] = [&device]() {
                auto createShaderModule = [&device](std::byte* spirv, size_t sz) {
                    return device.createShaderModule({.codeSize{sz}, .pCode{reinterpret_cast<uint32_t*>(spirv)}});
                };
                std::unique_ptr<std::byte[]> vertexShaderBinary, fragmentShaderBinary;
                size_t vertexShaderByteLength, fragmentShaderByteLength;
                std::tie(vertexShaderBinary, vertexShaderByteLength) = read_binary_file(
                    APP_VERTEX_SHADER_PATH);  // NOTE: for some reason MSVC doesn't like a structured binding here
                std::tie(fragmentShaderBinary, fragmentShaderByteLength) = read_binary_file(APP_FRAGMENT_SHADER_PATH);
                auto vertexShaderModule = createShaderModule(vertexShaderBinary.get(), vertexShaderByteLength);
                auto fragmentShaderModule = createShaderModule(fragmentShaderBinary.get(), fragmentShaderByteLength);
                return std::tuple{vertexShaderModule, fragmentShaderModule};
            }();
            auto shaderStageCreateInfos =
                std::array{vk::PipelineShaderStageCreateInfo{
                               .stage{vk::ShaderStageFlagBits::eVertex},
                               .module{vertexShaderModule},
                               .pName{APP_VERTEX_SHADER_ENTRY_POINT},
                               .pSpecializationInfo{}  // NOTE: can be used for constants in shader code, like work group size
                           },
                           vk::PipelineShaderStageCreateInfo{.stage{vk::ShaderStageFlagBits::eFragment},
                                                             .module{fragmentShaderModule},
                                                             .pName{APP_FRAGMENT_SHADER_ENTRY_POINT},
                                                             .pSpecializationInfo{}}};
            vk::PipelineVertexInputStateCreateInfo vertexInputState{};  // NOTE: should use vertex and index buffers
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{.topology = vk::PrimitiveTopology::eTriangleList,
                                                                        .primitiveRestartEnable = false};
            // NOTE: vk::PipelineTessellationStateCreateInfo is used with tesselation enabled
            vk::Viewport viewport{.x = 0,
                                  .y = 0,
                                  .width = static_cast<float>(swapchainImageExtent.width),
                                  .height = static_cast<float>(swapchainImageExtent.height),
                                  .minDepth = 0.0,
                                  .maxDepth = 1.0};
            vk::Rect2D scissor{.offset{0, 0}, .extent{swapchainImageExtent}};
            vk::PipelineViewportStateCreateInfo viewportState{
                .viewportCount = 1,  // NOTE: using multiple requires enabling a device feature
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor};
            vk::PipelineRasterizationStateCreateInfo rasterizationState{
                .depthClampEnable = false,              // NOTE: useful for shadow mapping, requires a device feature
                .rasterizerDiscardEnable = false,       // enabling it discards all fragments (causes no output)
                .polygonMode = vk::PolygonMode::eFill,  // NOTE: using eLine for wireframe requires a device feature
                .cullMode = vk::CullModeFlagBits::eBack,
                .frontFace = vk::FrontFace::eClockwise,
                .depthBiasEnable = false,  // NOTE: this and similar useful for shadow mapping
                .lineWidth = 1.0           // a wider line requires a device feature
            };
            vk::PipelineMultisampleStateCreateInfo multisampleState{// NOTE: can be use for AA
                                                                    .rasterizationSamples = APP_SAMPLE_COUNT};
            // NOTE: vk::PipelineDepthStencilStateCreateInfo is used when a depth/stencil buffer is present
            vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{
                // NOTE: can be disabled if not using blending
                .blendEnable = true,
                .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
                .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
                .colorBlendOp = vk::BlendOp::eAdd,
                .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
                .alphaBlendOp = vk::BlendOp::eAdd,
                .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                  vk::ColorComponentFlagBits::eB |
                                  vk::ColorComponentFlagBits::eA  // NOTE: when this initializer was missing there was no output
            };
            vk::PipelineColorBlendStateCreateInfo colorBlendState{
                .logicOpEnable = false,  // NOTE: can be used for bitwise compositing, possibly in OIT
                .attachmentCount = 1,    // NOTE: can use multiplt for multiple target; different options require a device feature
                .pAttachments = &colorBlendAttachmentState
                // NOTE .blendConstants can be used for custom blend constants in blend operations
            };
            // NOTE: vk::PipelineDynamicStateCreateInfo can be used for dynamic state
            vk::PipelineLayout pipelineLayout = device.createPipelineLayout({});  // NOTE: used with uniforms and push constants

            vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
                .flags{},
                // NOTE: can be used to disable optimizations, enable derivative pipelines and VK_NV_device_generated_commands
                .stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size()),
                .pStages = shaderStageCreateInfos.data(),
                .pVertexInputState = &vertexInputState,
                .pInputAssemblyState = &inputAssemblyState,
                .pTessellationState = nullptr,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizationState,
                .pMultisampleState = &multisampleState,
                .pColorBlendState = &colorBlendState,
                .pDynamicState = nullptr,
                .layout = pipelineLayout,
                .renderPass = renderpass,
                .subpass = APP_GRAPHICS_PIPELINE_SUBPASS_INDEX,
                .basePipelineHandle = VK_NULL_HANDLE  // NOTE: can be used to derive from an existing pipeline
                                                      // to speed up pipeline creation time
            };

            vk::PipelineCache pipelineCache = VK_NULL_HANDLE;  // NOTE: load cache from disk to speed up creation time
            auto [result, pipeline] = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo);
            if (result != vk::Result::eSuccess) {
                // NOTE: a possible good value is VK_PIPELINE_COMPILE_REQUIRED_EXT
                throw std::runtime_error("Couldn't create the graphics pipeline");
            }

            device.destroy(vertexShaderModule);
            device.destroy(fragmentShaderModule);
            return std::tuple{std::move(pipeline), std::move(pipelineLayout)};
        }();

        // Record command buffers; NOTE: usually this isn't preprocessed, but done every frame
        [&renderpass, &framebuffers, &swapchainImageExtent, &commandBuffers, &graphicsPipeline]() {
            for (size_t i = 0; i < commandBuffers.size(); ++i) {
                auto&& commandBuffer = commandBuffers[i];
                commandBuffer.begin({
                    .flags{},  // NOTE: VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT can be used when frequently recording
                    .pInheritanceInfo = nullptr  // NOTE: used by secondary command buffers to inherit state from primary ones
                });
                commandBuffer.beginRenderPass2(
                    {
                        .renderPass = renderpass,
                        .framebuffer = framebuffers[i],
                        .renderArea = {.offset = {0, 0}, .extent = swapchainImageExtent},
                        .clearValueCount = 0  // NOTE: used when there are any clearing operations
                    },
                    {
                        .contents =
                            vk::SubpassContents::eInline  // NOTE: can be used to source from secondary command buffers instead
                    });

                // NOTE: Actual rendering commands
                commandBuffer.bindPipeline(APP_SUBPASS_PIPELINE_BIND_POINT, graphicsPipeline);
                commandBuffer.draw(3, 1, 0, 0);  // NOTE: magic numbers

                commandBuffer.endRenderPass2(vk::SubpassEndInfo{});

                commandBuffer.end();
            }
        }();

        // Main loop
        [&window, &physicalDeviceGroup, &device, &commandBuffers, &graphicsQueue, &swapchain, &maxFramesInFlight,
         &currentImageAcquiredSemaphores, &imageRenderedSemaphores, &imageRenderedFences,
         swapchainImageCount = swapchainImages.size()]() {
            std::vector<vk::Semaphore> availableImageAcquiredSemaphores;
            auto getNextImageAcquiredSemaphore = [&device, &availableImageAcquiredSemaphores]() {
                if (availableImageAcquiredSemaphores.empty()) {
                    return device.createSemaphore(vk::SemaphoreCreateInfo{});
                }
                auto semaphore = availableImageAcquiredSemaphores.back();
                availableImageAcquiredSemaphores.pop_back();
                return semaphore;
            };

            while (!window.shouldClose()) {
                glfw::pollEvents();
                // Acquire the image to render to from the swapchain
                auto imageAcquiredSemaphore = getNextImageAcquiredSemaphore();
                auto imageIndex = device.acquireNextImage2KHR(
                    vk::AcquireNextImageInfoKHR{.swapchain = swapchain,
                                                .timeout = std::numeric_limits<uint64_t>::max(),
                                                .semaphore = imageAcquiredSemaphore,
                                                .deviceMask = (1u << physicalDeviceGroup.physicalDeviceCount) - 1u});
                device.waitForFences(imageRenderedFences[imageIndex], true, std::numeric_limits<uint64_t>::max());
                device.resetFences(imageRenderedFences[imageIndex]);
                if (currentImageAcquiredSemaphores[imageIndex]) {
                    availableImageAcquiredSemaphores.push_back(currentImageAcquiredSemaphores[imageIndex]);
                }
                currentImageAcquiredSemaphores[imageIndex] = imageAcquiredSemaphore;

                // Submit rendering commands to the GPU
                vk::PipelineStageFlags waitStageBits = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                graphicsQueue.submit(vk::SubmitInfo{.waitSemaphoreCount = 1,
                                                    .pWaitSemaphores = &imageAcquiredSemaphore,
                                                    .pWaitDstStageMask = &waitStageBits,
                                                    .commandBufferCount = 1,
                                                    .pCommandBuffers = &commandBuffers[imageIndex],
                                                    .signalSemaphoreCount = 1,
                                                    .pSignalSemaphores = &imageRenderedSemaphores[imageIndex]},
                                     imageRenderedFences[imageIndex]);
                graphicsQueue.presentKHR(vk::PresentInfoKHR{.waitSemaphoreCount = 1,
                                                            .pWaitSemaphores = &imageRenderedSemaphores[imageIndex],
                                                            .swapchainCount = 1,
                                                            .pSwapchains = &swapchain,
                                                            .pImageIndices = &imageIndex.value});
            }
            device.waitIdle();

            for (auto&& s : availableImageAcquiredSemaphores)
                device.destroy(s);
        }();

        // Cleanup
        for (auto&& framebuffer : framebuffers) {
            device.destroy(framebuffer);
        }
        device.destroy(graphicsPipeline);
        device.destroy(graphicsPipelineLayout);
        device.destroy(renderpass);
        for (auto&& swapchainImageView : swapchainImageViews) {
            device.destroy(swapchainImageView);
        }
        for (auto&& imageRenderedSemaphore : imageRenderedSemaphores) {
            device.destroy(imageRenderedSemaphore);
        }
        for (auto&& imageRenderedFence : imageRenderedFences) {
            device.destroy(imageRenderedFence);
        }
        for (auto&& currentImageAcquiredSemaphore : currentImageAcquiredSemaphores) {
            if (currentImageAcquiredSemaphore) {
                device.destroy(currentImageAcquiredSemaphore);
            }
        }
        device.destroy(swapchain);
        device.destroy(commandPool);
        device.destroy();
        instance.destroy(surface);
        instance.destroy(APP_ALLOCATION_CALLBACKS);
    } catch (const glfw::Error& e) {
        std::cerr << "GLFW error: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const vk::Error& e) {
        std::cerr << "Vulkan error: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}