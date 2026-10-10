#include "prospero/modules/libSceVideoOut.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/gpu/vulkan_backend.h"
#include "prospero/common/logging.h"
#include <windows.h>
#include <chrono>

namespace prospero::modules {

static LRESULT CALLBACK VideoOutWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CLOSE || msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LibSceVideoOut::LibSceVideoOut() : HleModule("libSceVideoOut.sprx") {}

LibSceVideoOut::~LibSceVideoOut() {
    closeDisplayWindow();
}

bool LibSceVideoOut::openDisplayWindow(u32 width, u32 height, const std::string& title) {
    if (m_window_open && m_hwnd) return true;

    m_fb_width = width ? width : 1280;
    m_fb_height = height ? height : 720;
    m_base_title = title;

    WNDCLASSA wc{};
    wc.lpfnWndProc = VideoOutWndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "ProsperoVideoOutWindow";
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassA(&wc);

    RECT wr = {0, 0, static_cast<LONG>(m_fb_width), static_cast<LONG>(m_fb_height)};
    AdjustWindowRect(&wr, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int pos_x = (screen_w - (wr.right - wr.left)) / 2;
    int pos_y = (screen_h - (wr.bottom - wr.top)) / 2;

    HWND hwnd = CreateWindowA(
        "ProsperoVideoOutWindow",
        m_base_title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        pos_x, pos_y,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );

    if (!hwnd) {
        PROSPERO_LOG_ERROR("VIDEO_OUT", "Failed to create Win32 display window");
        return false;
    }

    m_hwnd = hwnd;
    m_hdc = GetDC(hwnd);
    m_window_open = true;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    PROSPERO_LOG_INFO("VIDEO_OUT", "Opened display window (" << m_fb_width << "x" << m_fb_height << ") for PS5 video presentation");
    return true;
}

void LibSceVideoOut::closeDisplayWindow() {
    if (m_hwnd) {
        if (m_hdc) {
            ReleaseDC(static_cast<HWND>(m_hwnd), static_cast<HDC>(m_hdc));
            m_hdc = nullptr;
        }
        DestroyWindow(static_cast<HWND>(m_hwnd));
        m_hwnd = nullptr;
    }
    m_window_open = false;
}

bool LibSceVideoOut::processWindowMessages() {
    if (!m_hwnd || !m_window_open) return false;

    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_window_open = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return m_window_open;
}

void LibSceVideoOut::renderFrame(const void* pixel_data, u32 width, u32 height) {
    if (!m_hwnd || !m_hdc || !pixel_data) return;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -static_cast<LONG>(height); // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(
        static_cast<HDC>(m_hdc),
        0, 0, width, height,
        0, 0, width, height,
        pixel_data,
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

bool LibSceVideoOut::init() {
    // sceVideoOutOpen (0x3BC6AE90F3B20110)
    registerExport(nids::sceVideoOutOpen, "sceVideoOutOpen",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 userId = static_cast<u32>(a1);
            s32 type = static_cast<s32>(a2);
            s32 index = static_cast<s32>(a3);
            PROSPERO_LOG_INFO("HLE_VIDEO", "sceVideoOutOpen: User=" << userId 
                              << " Type=" << type << " Index=" << index << " -> Handle=0x100");
            openDisplayWindow(m_fb_width, m_fb_height);
            return 0x100; // Simulated VideoOut handle
        });

    // sceVideoOutClose (0x4386780BCF6522E1)
    registerExport(nids::sceVideoOutClose, "sceVideoOutClose",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            PROSPERO_LOG_INFO("HLE_VIDEO", "sceVideoOutClose: Handle=0x" << std::hex << handle << std::dec);
            closeDisplayWindow();
            return SCE_OK;
        });

    // sceVideoOutRegisterBuffers (0x71BC39218F5B891A)
    registerExport(nids::sceVideoOutRegisterBuffers, "sceVideoOutRegisterBuffers",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            s32 startIndex = static_cast<s32>(a2);
            vaddr_t addresses = a3;
            s32 bufferNum = static_cast<s32>(a4);
            vaddr_t param = a5;

            PROSPERO_LOG_INFO("HLE_VIDEO", "sceVideoOutRegisterBuffers: Handle=0x" << std::hex << handle 
                              << " Buffers=" << bufferNum << " StartIndex=" << startIndex << std::dec);

            if (addresses != 0 && m_framebuffer_vaddr == 0) {
                m_framebuffer_vaddr = addresses;
            }

            if (!m_window_open) {
                openDisplayWindow(m_fb_width, m_fb_height);
            }

            return SCE_OK;
        });

    // sceVideoOutSubmitFlip (0x93FA56E1AC90881B)
    registerExport(nids::sceVideoOutSubmitFlip, "sceVideoOutSubmitFlip",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            s32 bufferIndex = static_cast<s32>(a2);
            s32 flipMode = static_cast<s32>(a3);
            s64 flipArg = static_cast<s64>(a4);

            m_flip_count++;
            processWindowMessages();

            // Calculate FPS
            static auto s_fps_start = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - s_fps_start).count();
            m_frames_in_interval++;

            if (elapsed >= 0.5) {
                m_current_fps = static_cast<float>(m_frames_in_interval / elapsed);
                m_frames_in_interval = 0;
                s_fps_start = now;

                if (m_hwnd) {
                    const auto& gpu_info = gpu::VulkanBackend::instance().getSelectedDevice();
                    char title_buf[256];
                    snprintf(title_buf, sizeof(title_buf),
                             "ProsperoCore PS5 | %s | %.1f FPS (%.1f ms) | Flips: %llu",
                             gpu_info.device_name.empty() ? "NVIDIA RTX 3090" : gpu_info.device_name.c_str(),
                             m_current_fps,
                             m_current_fps > 0.0f ? (1000.0f / m_current_fps) : 0.0f,
                             m_flip_count);
                    SetWindowTextA(static_cast<HWND>(m_hwnd), title_buf);
                }
            }

            if (m_framebuffer_vaddr) {
                void* pixels = memory::MemoryManager::instance().getPointer(m_framebuffer_vaddr);
                if (pixels && m_hwnd) {
                    renderFrame(pixels, m_fb_width, m_fb_height);
                }
            }

            if (m_flip_count % 300 == 0 || m_flip_count <= 2) {
                PROSPERO_LOG_GPU("VIDEO_OUT", "Flip #" << m_flip_count << " (Buffer=" << bufferIndex 
                                 << " Mode=" << flipMode << " FPS=" << m_current_fps << ")");
            }
            return SCE_OK;
        });

    // sceVideoOutGetResolutionStatus (0x18F45A9CEB1046A0)
    registerExport(nids::sceVideoOutGetResolutionStatus, "sceVideoOutGetResolutionStatus",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            vaddr_t status_ptr = a2;

            struct SceVideoOutResolutionStatus {
                u32 width;
                u32 height;
                u32 refresh_rate;
                u32 flags;
            } status{1280, 720, 60, 0}; // 720p 60Hz default

            if (status_ptr) {
                memory::MemoryManager::instance().write(status_ptr, &status, sizeof(status));
            }
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
