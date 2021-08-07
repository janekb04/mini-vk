#include <mimalloc-new-delete.h> // override all allocations with the optimized mimalloc allocator library; NOTE: consider calling mi_option_set for some performance tweaks

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1 // load Vulkan dynamically rather than statically
#define VULKAN_HPP_NO_CONSTRUCTORS // use C++20's designated initializers
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // as above; NOTE: for compatibility with older SDKs
#define VULKAN_HPP_NO_SETTERS // disable setter methods as unneeded and unnecessarily extending compilation time
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR // use C++20's spaceship operator; NOTE: for compatibility with older SDKs
#include <vulkan/vulkan.hpp> // use the C++ bindings for Vulkan instead of the C headers; NOTE: there is also a higher level wrapper called vulkan_raii.hpp with a different interface
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE // has to be defined exactly once when using VULKAN_HPP_DISPATCH_LOADER_DYNAMIC

#include <glfwpp/glfwpp.h> // use my (janekb04 at Github) C++ wrapper for GLFW 

#include <iostream> // for logging

const int WND_WIDTH = 800, WND_HEIGHT = 600; // window dimensions (for now non-resizable)
const char* const APP_NAME = "vk_mini";
const uint32_t APP_API_VERSION = VK_MAKE_API_VERSION(0, 1, 2, 0);
const std::array APP_VALIDATION_LAYERS = {
#ifdef NDEBUG
    // intentionally left blank
#else
    "VK_LAYER_KHRONOS_validation"
#endif
};
const vk::AllocationCallbacks APP_ALLOCATION_CALLBACKS{ // NOTE: consider not using allocation callbacks as the performance gain/loss hasn't been measured. They are intended for logging rather than for a performance gain.
    .pUserData = nullptr,
    .pfnAllocation = [](void* /*pUserData*/, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/) {
        void* p = mi_malloc_aligned(size, alignment);
        return p;
    },
    .pfnReallocation = [](void* /*pUserData*/, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/) {
        return mi_realloc_aligned(pOriginal, size, alignment);
    },
    .pfnFree = [](void* /*pUserData*/, void* pMemory) {
        mi_free(pMemory);
    }
    // NOTE: callbacks can be installed for internal allocations. They are only callbacks that will notify the application that the Vulkan implementation performed an allocation using its own mechanisms.
};

int main()
{
    try
    {
        // Initialize GLFW and create window
        auto GLFW = glfw::init();
        glfw::WindowHints{
            .resizable = false,
            .clientApi = glfw::ClientApi::None
        }.apply();
        glfw::Window window{WND_WIDTH, WND_HEIGHT, APP_NAME};

        // Load global Vulkan functions
        vk::DynamicLoader dl; // has destructor
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        // Create Vulkan instance
        vk::Instance instance = [](){
            uint32_t implementation_api_version = VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion ? vk::enumerateInstanceVersion() : VK_API_VERSION_1_0;
            
            if (implementation_api_version < APP_API_VERSION)
            {
                throw std::runtime_error("The Vulkan implementation doesn't support the version required by the application");
            }

            vk::ApplicationInfo appInfo{
                .pApplicationName = APP_NAME,
                .applicationVersion = 1,
                .pEngineName = APP_NAME,
                .engineVersion = 1,
                .apiVersion = APP_API_VERSION
            };
        
            auto instanceExtensions = glfw::getRequiredInstanceExtensions();

            return vk::createInstance(vk::InstanceCreateInfo{ // NOTE: use pNext to debug instance creation and destruction
                    .pApplicationInfo = &appInfo,
                    .enabledLayerCount = APP_VALIDATION_LAYERS.size(),
                    .ppEnabledLayerNames = APP_VALIDATION_LAYERS.data(), // NOTE: vk::EnumerateInstanceLayerProperties to handle optional layers
                    .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()), // NOTE: vk::EnumerateInstanceExtensionProperties to handle optional extensions
                    .ppEnabledExtensionNames = instanceExtensions.data()
                }, 
                APP_ALLOCATION_CALLBACKS // it is sufficient to pass the allocation callbacks only here and at .destroy(), they will be used by all child objects of this instance as per the documentation of VkSystemAllocationScope
            );
        }();

        // NOTE: create a VkDebugUtilsMessengerEXT for custom error callback instead of default print to stdout
        
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance); // load instance-specific function pointers

        auto physicalDeviceGroups = instance.enumeratePhysicalDeviceGroups();
        if (physicalDeviceGroups.empty())
        {
            throw std::runtime_error("No compatible Physical Devices (GPUs) detected");
        }
        auto physicalDevice = physicalDeviceGroups[0].physicalDevices[0];
        
        // NOTE: vk::GetPhysicalDeviceProperties and

        //VULKAN_HPP_DEFAULT_DISPATCHER.init(device); // load device-specific function pointers

        while (!window.shouldClose())
        {
            glfw::pollEvents();
        }

        //instance.destroy(APP_ALLOCATION_CALLBACKS);
    }
    catch(const glfw::Error& e)
    {
        std::cerr << "GLFW error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch(const vk::Error& e)
    {
        std::cerr << "Vulkan error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << "Runtime Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return EXIT_FAILURE; 
    }
    catch(...)
    {
        std::cerr << "Unknown error\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}