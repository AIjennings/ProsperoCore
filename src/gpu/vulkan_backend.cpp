#include "prospero/gpu/vulkan_backend.h"
#include "prospero/common/logging.h"
#include <windows.h>
#include <iostream>
#include <iomanip>

namespace prospero::gpu {

namespace vk {
    constexpr int VK_STRUCTURE_TYPE_APPLICATION_INFO = 0;
    constexpr int VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1;
    constexpr int VK_SUCCESS = 0;
    constexpr u32 VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2;
    constexpr u32 VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = 0x00000001;

    struct VkApplicationInfo {
        int sType;
        const void* pNext;
        const char* pApplicationName;
        u32 applicationVersion;
        const char* pEngineName;
        u32 engineVersion;
        u32 apiVersion;
    };

    struct VkInstanceCreateInfo {
        int sType;
        const void* pNext;
        u32 flags;
        const VkApplicationInfo* pApplicationInfo;
        u32 enabledLayerCount;
        const char* const* ppEnabledLayerNames;
        u32 enabledExtensionCount;
        const char* const* ppEnabledExtensionNames;
    };

    struct VkPhysicalDeviceLimits {
        u8 dummy[504];
    };

    struct VkPhysicalDeviceSparseProperties {
        u32 dummy[5];
    };

    struct VkPhysicalDeviceProperties {
        u32 apiVersion;
        u32 driverVersion;
        u32 vendorID;
        u32 deviceID;
        u32 deviceType;
        char deviceName[256];
        u8 pipelineCacheUUID[16];
        VkPhysicalDeviceLimits limits;
        VkPhysicalDeviceSparseProperties sparseProperties;
    };

    struct VkMemoryType {
        u32 propertyFlags;
        u32 heapIndex;
    };

    struct VkMemoryHeap {
        u64 size;
        u32 flags;
    };

    struct VkPhysicalDeviceMemoryProperties {
        u32 memoryTypeCount;
        VkMemoryType memoryTypes[32];
        u32 memoryHeapCount;
        VkMemoryHeap memoryHeaps[16];
    };

    using PFN_vkGetInstanceProcAddr = void* (*)(void* instance, const char* pName);
    using PFN_vkEnumerateInstanceVersion = int (*)(u32* pApiVersion);
    using PFN_vkCreateInstance = int (*)(const VkInstanceCreateInfo* pCreateInfo, const void* pAllocator, void** pInstance);
    using PFN_vkDestroyInstance = void (*)(void* instance, const void* pAllocator);
    using PFN_vkEnumeratePhysicalDevices = int (*)(void* instance, u32* pPhysicalDeviceCount, void** pPhysicalDevices);
    using PFN_vkGetPhysicalDeviceProperties = void (*)(void* physicalDevice, VkPhysicalDeviceProperties* pProperties);
    using PFN_vkGetPhysicalDeviceMemoryProperties = void (*)(void* physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties);
} // namespace vk

VulkanBackend::~VulkanBackend() {
    shutdown();
}

bool VulkanBackend::loadVulkanLibrary() {
    m_vulkan_module = LoadLibraryA("vulkan-1.dll");
    if (!m_vulkan_module) {
        PROSPERO_LOG_WARN("GPU", "vulkan-1.dll not found on host system");
        return false;
    }
    return true;
}

bool VulkanBackend::init() {
    PROSPERO_LOG_INFO("GPU", "Initializing Vulkan Graphics Translation Layer...");

    if (!loadVulkanLibrary()) return false;

    auto vkGetInstanceProcAddr = reinterpret_cast<vk::PFN_vkGetInstanceProcAddr>(
        GetProcAddress(reinterpret_cast<HMODULE>(m_vulkan_module), "vkGetInstanceProcAddr"));

    if (!vkGetInstanceProcAddr) {
        PROSPERO_LOG_ERROR("GPU", "Failed to resolve vkGetInstanceProcAddr");
        return false;
    }

    auto vkEnumerateInstanceVersion = reinterpret_cast<vk::PFN_vkEnumerateInstanceVersion>(
        vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    
    if (vkEnumerateInstanceVersion) {
        vkEnumerateInstanceVersion(&m_vulkan_version);
        u32 major = m_vulkan_version >> 22;
        u32 minor = (m_vulkan_version >> 12) & 0x3FF;
        u32 patch = m_vulkan_version & 0xFFF;
        PROSPERO_LOG_INFO("GPU", "Host Vulkan Loader Version: " << major << "." << minor << "." << patch);
    }

    auto vkCreateInstance = reinterpret_cast<vk::PFN_vkCreateInstance>(
        vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
    auto vkDestroyInstance = reinterpret_cast<vk::PFN_vkDestroyInstance>(
        vkGetInstanceProcAddr(nullptr, "vkDestroyInstance"));

    if (!vkCreateInstance) {
        PROSPERO_LOG_ERROR("GPU", "Failed to resolve vkCreateInstance");
        return false;
    }

    vk::VkApplicationInfo appInfo{};
    appInfo.sType = vk::VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ProsperoCore PS5 Emulator";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "ProsperoVulkan";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = m_vulkan_version ? m_vulkan_version : (1 << 22 | 3 << 12);

    vk::VkInstanceCreateInfo createInfo{};
    createInfo.sType = vk::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    void* instance = nullptr;
    int res = vkCreateInstance(&createInfo, nullptr, &instance);
    if (res != vk::VK_SUCCESS || !instance) {
        PROSPERO_LOG_ERROR("GPU", "vkCreateInstance failed with error: " << res);
        return false;
    }

    auto vkEnumeratePhysicalDevices = reinterpret_cast<vk::PFN_vkEnumeratePhysicalDevices>(
        vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
    auto vkGetPhysicalDeviceProperties = reinterpret_cast<vk::PFN_vkGetPhysicalDeviceProperties>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
    auto vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<vk::PFN_vkGetPhysicalDeviceMemoryProperties>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties"));

    if (vkEnumeratePhysicalDevices && vkGetPhysicalDeviceProperties) {
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount > 0) {
            std::vector<void*> physicalDevices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

            for (void* pdev : physicalDevices) {
                std::vector<u8> prop_buffer(8192, 0);
                auto* props = reinterpret_cast<vk::VkPhysicalDeviceProperties*>(prop_buffer.data());
                vkGetPhysicalDeviceProperties(pdev, props);

                GpuDeviceInfo info{};
                info.device_name = props->deviceName;
                info.api_version = props->apiVersion;
                info.driver_version = props->driverVersion;
                info.vendor_id = props->vendorID;
                info.device_id = props->deviceID;
                info.is_discrete = (props->deviceType == vk::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

                if (vkGetPhysicalDeviceMemoryProperties) {
                    std::vector<u8> mem_buffer(8192, 0);
                    auto* memProps = reinterpret_cast<vk::VkPhysicalDeviceMemoryProperties*>(mem_buffer.data());
                    vkGetPhysicalDeviceMemoryProperties(pdev, memProps);
                    for (u32 h = 0; h < memProps->memoryHeapCount; ++h) {
                        if (memProps->memoryHeaps[h].flags & vk::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                            info.vram_bytes += memProps->memoryHeaps[h].size;
                        }
                    }
                }

                m_available_devices.push_back(info);

                // Select discrete GPU (e.g. RTX 3090)
                if (info.is_discrete || m_selected_device.device_name.empty()) {
                    m_selected_device = info;
                }
            }
        }
    }

    PROSPERO_LOG_INFO("GPU", "Active GPU: " << m_selected_device.device_name 
                      << " (VRAM: " << (m_selected_device.vram_bytes / (1024 * 1024)) << " MB)");

    if (vkDestroyInstance && instance) {
        vkDestroyInstance(instance, nullptr);
    }

    m_initialized = true;
    return true;
}

void VulkanBackend::shutdown() {
    if (m_vulkan_module) {
        FreeLibrary(reinterpret_cast<HMODULE>(m_vulkan_module));
        m_vulkan_module = nullptr;
    }
    m_initialized = false;
}

void VulkanBackend::dumpInfo() const {
    std::cout << "\n=======================================================\n";
    std::cout << "             PROSPERO VULKAN GPU BACKEND               \n";
    std::cout << "=======================================================\n";
    std::cout << "  Status:         " << (m_initialized ? "Initialized (Active)" : "Offline") << "\n";
    std::cout << "  Device Name:    " << m_selected_device.device_name << "\n";
    std::cout << "  Vendor ID:      0x" << std::hex << m_selected_device.vendor_id << std::dec << "\n";
    std::cout << "  Device ID:      0x" << std::hex << m_selected_device.device_id << std::dec << "\n";
    std::cout << "  Device Type:    " << (m_selected_device.is_discrete ? "Discrete GPU" : "Integrated GPU") << "\n";
    std::cout << "  Dedicated VRAM: " << (m_selected_device.vram_bytes / (1024 * 1024 * 1024.0)) << " GiB\n";
    std::cout << "  Available GPUs: " << m_available_devices.size() << "\n";
    for (size_t i = 0; i < m_available_devices.size(); ++i) {
        std::cout << "    [" << i << "] " << m_available_devices[i].device_name << "\n";
    }
    std::cout << "=======================================================\n\n";
}

} // namespace prospero::gpu
