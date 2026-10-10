#include "prospero/modules/libScePad.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <windows.h>
#include <chrono>

namespace prospero::modules {

u8 LibScePad::s_large_motor = 0;
u8 LibScePad::s_small_motor = 0;

LibScePad::LibScePad() : HleModule("libScePad.sprx") {}

void LibScePad::setHapticVibration(u8 large_motor, u8 small_motor) {
    s_large_motor = large_motor;
    s_small_motor = small_motor;

    // Direct XInput vibration if controller connected
    static HMODULE hXInput = LoadLibraryA("xinput1_4.dll");
    if (!hXInput) hXInput = LoadLibraryA("xinput9_1_0.dll");
    if (hXInput) {
        typedef struct { WORD wLeftMotorSpeed; WORD wRightMotorSpeed; } XINPUT_VIB;
        typedef DWORD (WINAPI *PFN_XSet)(DWORD, XINPUT_VIB*);
        static auto pfnSet = (PFN_XSet)GetProcAddress(hXInput, "XInputSetState");
        if (pfnSet) {
            XINPUT_VIB vib{};
            vib.wLeftMotorSpeed = static_cast<WORD>(large_motor) * 257;
            vib.wRightMotorSpeed = static_cast<WORD>(small_motor) * 257;
            pfnSet(0, &vib);
        }
    }
}

void LibScePad::getHapticState(u8& large_motor, u8& small_motor) {
    large_motor = s_large_motor;
    small_motor = s_small_motor;
}

void LibScePad::pollHostInput(ScePadData& pad) {
    pad = ScePadData{};
    pad.connected = 1;
    pad.left_stick_x = 128;
    pad.left_stick_y = 128;
    pad.right_stick_x = 128;
    pad.right_stick_y = 128;

    static auto s_epoch = std::chrono::steady_clock::now();
    pad.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - s_epoch).count();

    // Check XInput Gamepad (Xbox / DualSense DS4Windows)
    static HMODULE hXInput = LoadLibraryA("xinput1_4.dll");
    if (!hXInput) hXInput = LoadLibraryA("xinput9_1_0.dll");
    if (hXInput) {
        typedef struct {
            DWORD dwPacketNumber;
            struct {
                WORD wButtons;
                BYTE bLeftTrigger;
                BYTE bRightTrigger;
                SHORT sThumbLX;
                SHORT sThumbLY;
                SHORT sThumbRX;
                SHORT sThumbRY;
            } Gamepad;
        } XINPUT_STATE;
        typedef DWORD (WINAPI *PFN_XGet)(DWORD, XINPUT_STATE*);
        static auto pfnGet = (PFN_XGet)GetProcAddress(hXInput, "XInputGetState");
        if (pfnGet) {
            XINPUT_STATE state{};
            if (pfnGet(0, &state) == 0) { // Connected
                // Convert signed short [-32768, 32767] to [0, 255]
                int lx = (state.Gamepad.sThumbLX + 32768) >> 8;
                int ly = 255 - ((state.Gamepad.sThumbLY + 32768) >> 8);
                if (abs(lx - 128) > 16) pad.left_stick_x = static_cast<u8>(lx);
                if (abs(ly - 128) > 16) pad.left_stick_y = static_cast<u8>(ly);

                pad.analog_l2 = state.Gamepad.bLeftTrigger;
                pad.analog_r2 = state.Gamepad.bRightTrigger;

                if (state.Gamepad.wButtons & 0x0001) pad.buttons |= SCE_PAD_UP;
                if (state.Gamepad.wButtons & 0x0002) pad.buttons |= SCE_PAD_DOWN;
                if (state.Gamepad.wButtons & 0x0004) pad.buttons |= SCE_PAD_LEFT;
                if (state.Gamepad.wButtons & 0x0008) pad.buttons |= SCE_PAD_RIGHT;
                if (state.Gamepad.wButtons & 0x0010) pad.buttons |= SCE_PAD_OPTIONS;
                if (state.Gamepad.wButtons & 0x1000) pad.buttons |= SCE_PAD_CROSS;
                if (state.Gamepad.wButtons & 0x2000) pad.buttons |= SCE_PAD_CIRCLE;
                if (state.Gamepad.wButtons & 0x4000) pad.buttons |= SCE_PAD_SQUARE;
                if (state.Gamepad.wButtons & 0x8000) pad.buttons |= SCE_PAD_TRIANGLE;
                if (state.Gamepad.bRightTrigger > 30) pad.buttons |= SCE_PAD_R2;
                if (state.Gamepad.bLeftTrigger > 30) pad.buttons |= SCE_PAD_L2;
                if (state.Gamepad.wButtons & 0x0100) pad.buttons |= SCE_PAD_L1;
                if (state.Gamepad.wButtons & 0x0200) pad.buttons |= SCE_PAD_R1;
            }
        }
    }

    // Keyboard Fallback / Overlay
    auto isDown = [](int vk) -> bool {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    };

    if (isDown(VK_LEFT) || isDown('A')) {
        pad.left_stick_x = 0;
        pad.buttons |= SCE_PAD_LEFT;
    }
    if (isDown(VK_RIGHT) || isDown('D')) {
        pad.left_stick_x = 255;
        pad.buttons |= SCE_PAD_RIGHT;
    }
    if (isDown(VK_UP) || isDown('W')) {
        pad.left_stick_y = 0;
        pad.buttons |= SCE_PAD_UP;
    }
    if (isDown(VK_DOWN) || isDown('S')) {
        pad.left_stick_y = 255;
        pad.buttons |= SCE_PAD_DOWN;
    }
    if (isDown(VK_SPACE) || isDown('Z')) {
        pad.buttons |= SCE_PAD_CROSS;
    }
    if (isDown('X')) {
        pad.buttons |= SCE_PAD_CIRCLE;
    }
    if (isDown('C')) {
        pad.buttons |= SCE_PAD_SQUARE;
    }
    if (isDown('V')) {
        pad.buttons |= SCE_PAD_TRIANGLE;
    }
    if (isDown(VK_SHIFT)) {
        pad.buttons |= SCE_PAD_R2;
        pad.analog_r2 = 255;
    }
    if (isDown(VK_CONTROL)) {
        pad.buttons |= SCE_PAD_L2;
        pad.analog_l2 = 255;
    }
    if (isDown(VK_ESCAPE) || isDown('Q')) {
        pad.buttons |= SCE_PAD_OPTIONS;
    }
}

bool LibScePad::init() {
    // scePadOpen (0x5C9B387A6E9410A1)
    registerExport(nids::scePadOpen, "scePadOpen",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 userId = static_cast<u32>(a1);
            s32 type = static_cast<s32>(a2);
            s32 index = static_cast<s32>(a3);
            PROSPERO_LOG_INFO("HLE_PAD", "scePadOpen: User=" << userId << " -> Handle=0x201 (DualSense Wireless Controller)");
            return 0x201;
        });

    // scePadClose (0x6B18D37F098C7102)
    registerExport(nids::scePadClose, "scePadClose",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            PROSPERO_LOG_INFO("HLE_PAD", "scePadClose: Handle=0x" << std::hex << handle << std::dec);
            return SCE_OK;
        });

    // scePadReadState (0x2035F48293AA6D4B)
    registerExport(nids::scePadReadState, "scePadReadState",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            vaddr_t data_ptr = a2;

            ScePadData pad{};
            pollHostInput(pad);

            if (data_ptr) {
                memory::MemoryManager::instance().write(data_ptr, &pad, sizeof(pad));
            }
            return SCE_OK;
        });

    // scePadSetVibration (0x7E8159EACD34125B)
    registerExport(nids::scePadSetVibration, "scePadSetVibration",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            u8 large_motor = static_cast<u8>(a2);
            u8 small_motor = static_cast<u8>(a3);
            setHapticVibration(large_motor, small_motor);
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
