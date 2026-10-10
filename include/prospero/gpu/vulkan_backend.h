#pragma once

#include "prospero/common/types.h"
#include <string>
#include <vector>

namespace prospero::gpu {

struct GpuDeviceInfo {
    std::string device_name;
    u32 api_version{0};
    u32 driver_version{0};
    u32 vendor_id{0};
    u32 device_id{0};
    u64 vram_bytes{0};
    bool is_discrete{false};
};

class VulkanBackend {
public:
    static VulkanBackend& instance() {
        static VulkanBackend s_instance;
        return s_instance;
    }

    bool init();
    void shutdown();

    bool isAvailable() const { return m_initialized; }
    const GpuDeviceInfo& getSelectedDevice() const { return m_selected_device; }
    u32 getVulkanVersion() const { return m_vulkan_version; }

    void dumpInfo() const;

private:
    VulkanBackend() = default;
    ~VulkanBackend();

    bool loadVulkanLibrary();
    bool queryDevices();

    void* m_vulkan_module{nullptr};
    bool m_initialized{false};
    u32 m_vulkan_version{0};
    GpuDeviceInfo m_selected_device;
    std::vector<GpuDeviceInfo> m_available_devices;
};

} // namespace prospero::gpu
