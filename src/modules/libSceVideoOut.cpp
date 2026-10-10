#include "prospero/modules/libSceVideoOut.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"

namespace prospero::modules {

LibSceVideoOut::LibSceVideoOut() : HleModule("libSceVideoOut.sprx") {}

bool LibSceVideoOut::init() {
    // sceVideoOutOpen (0x3BC6AE90F3B20110)
    registerExport(nids::sceVideoOutOpen, "sceVideoOutOpen",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 userId = static_cast<u32>(a1);
            s32 type = static_cast<s32>(a2);
            s32 index = static_cast<s32>(a3);
            PROSPERO_LOG_INFO("HLE_VIDEO", "sceVideoOutOpen: User=" << userId 
                              << " Type=" << type << " Index=" << index << " -> Handle=0x100");
            return 0x100; // Simulated VideoOut handle
        });

    // sceVideoOutClose (0x4386780BCF6522E1)
    registerExport(nids::sceVideoOutClose, "sceVideoOutClose",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            PROSPERO_LOG_INFO("HLE_VIDEO", "sceVideoOutClose: Handle=0x" << std::hex << handle << std::dec);
            return SCE_OK;
        });

    // sceVideoOutRegisterBuffers (0x71BC39218F5B891A)
    registerExport(nids::sceVideoOutRegisterBuffers, "sceVideoOutRegisterBuffers",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            s32 startIndex = static_cast<s32>(a2);
            vaddr_t addresses = a3;
            s32 bufferNum = static_cast<s32>(a4);
            vaddr_t param = a5;

            PROSPERO_LOG_INFO("HLE_VIDEO", "sceVideoOutRegisterBuffers: Handle=0x" << std::hex << handle 
                              << " Buffers=" << bufferNum << " StartIndex=" << startIndex << std::dec);
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
            if (m_flip_count % 60 == 0 || m_flip_count <= 5) {
                PROSPERO_LOG_GPU("VIDEO_OUT", "Flip #" << m_flip_count << " (Buffer=" << bufferIndex 
                                 << " Mode=" << flipMode << ")");
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
            } status{3840, 2160, 60, 0}; // 4K 60Hz

            if (status_ptr) {
                memory::MemoryManager::instance().write(status_ptr, &status, sizeof(status));
            }
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
