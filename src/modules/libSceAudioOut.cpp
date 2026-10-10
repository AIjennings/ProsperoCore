#include "prospero/modules/libSceAudioOut.h"
#include "prospero/common/logging.h"

namespace prospero::modules {

LibSceAudioOut::LibSceAudioOut() : HleModule("libSceAudioOut.sprx") {}

bool LibSceAudioOut::init() {
    // sceAudioOutOpen (0x546B2C89A019E743)
    registerExport(nids::sceAudioOutOpen, "sceAudioOutOpen",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 userId = static_cast<u32>(a1);
            s32 type = static_cast<s32>(a2);
            s32 index = static_cast<s32>(a3);
            PROSPERO_LOG_INFO("HLE_AUDIO", "sceAudioOutOpen: User=" << userId << " Type=" << type << " -> Handle=0x301");
            return 0x301; // AudioOut Port Handle
        });

    // sceAudioOutClose (0x9C34FA6180E34BC1)
    registerExport(nids::sceAudioOutClose, "sceAudioOutClose",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 handle = static_cast<s32>(a1);
            PROSPERO_LOG_INFO("HLE_AUDIO", "sceAudioOutClose: Handle=0x" << std::hex << handle << std::dec);
            return SCE_OK;
        });

    // sceAudioOutOutput (0x7B9E3A56D089125C)
    registerExport(nids::sceAudioOutOutput, "sceAudioOutOutput",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            // Simulated audio PCM output
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
