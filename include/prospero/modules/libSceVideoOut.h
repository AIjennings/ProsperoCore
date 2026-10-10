#pragma once

#include "prospero/modules/module_manager.h"
#include <string>

namespace prospero::modules {

class LibSceVideoOut : public HleModule {
public:
    LibSceVideoOut();
    ~LibSceVideoOut();

    bool init() override;

    bool openDisplayWindow(u32 width = 1280, u32 height = 720, const std::string& title = "ProsperoCore - PlayStation 5 Emulator");
    void closeDisplayWindow();
    bool processWindowMessages();
    void renderFrame(const void* pixel_data, u32 width, u32 height);

    void registerFramebuffer(vaddr_t vaddr, u32 width, u32 height) {
        m_framebuffer_vaddr = vaddr;
        m_fb_width = width;
        m_fb_height = height;
    }

    bool isWindowOpen() const { return m_window_open; }
    void* getWindowHandle() const { return m_hwnd; }
    u64 getFlipCount() const { return m_flip_count; }
    float getCurrentFps() const { return m_current_fps; }
    u32 getWidth() const { return m_fb_width; }
    u32 getHeight() const { return m_fb_height; }

private:
    void* m_hwnd{nullptr};
    void* m_hdc{nullptr};
    bool m_window_open{false};
    u64 m_flip_count{0};
    vaddr_t m_framebuffer_vaddr{0};
    u32 m_fb_width{1280};
    u32 m_fb_height{720};
    std::string m_base_title;

    double m_last_fps_time{0.0};
    u64 m_frames_in_interval{0};
    float m_current_fps{60.0f};
};

} // namespace prospero::modules
