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
#include <iostream>
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

        // Create swapchain with images
        auto [swapchain, swapchainImageFormat, swapchainImageExtent,
              swapchainImages] = [&physicalDeviceGroup, &surface, &graphicsFamilyIdx, &presentFamilyIdx, &window, &device]() {
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
            auto compositeAlpha = window.getAttribTransparentFramebuffer() ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
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
            return std::tuple{std::move(swapchain), surfaceFormat.format, extent, device.getSwapchainImagesKHR(swapchain)};
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

        // Main loop
        while (!window.shouldClose()) {
            glfw::pollEvents();
        }

        // Cleanup
        for (auto&& swapchainImageView : swapchainImageViews) {
            device.destroy(swapchainImageView);
        }
        device.destroy(swapchain);
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