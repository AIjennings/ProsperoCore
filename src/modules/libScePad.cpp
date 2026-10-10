#include "prospero/modules/libScePad.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"

namespace prospero::modules {

LibScePad::LibScePad() : HleModule("libScePad.sprx") {}

bool LibScePad::init() {
    // scePadOpen (0x5C9B387A6E9410A1)
    registerExport(nids::scePadOpen, "scePadOpen",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 userId = static_cast<u32>(a1);
            s32 type = static_cast<s32>(a2);
            s32 index = static_cast<s32>(a3);
            PROSPERO_LOG_INFO("HLE_PAD", "scePadOpen: User=" << userId << " -> Handle=0x201 (DualSense 1)");
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
            pad.connected = 1;
            pad.left_stick_x = 128; // Centered
            pad.left_stick_y = 128;
            pad.right_stick_x = 128;
            pad.right_stick_y = 128;
            pad.buttons = 0; // Neutral

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
            PROSPERO_LOG_INFO("HLE_PAD", "DualSense Haptics: Large=" << (int)large_motor << " Small=" << (int)small_motor);
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
