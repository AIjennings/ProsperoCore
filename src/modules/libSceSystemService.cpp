#include "prospero/modules/libSceSystemService.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <chrono>

namespace prospero::modules {

LibSceSystemService::LibSceSystemService() : HleModule("libSceSystemService.sprx") {}

bool LibSceSystemService::init() {
    // sceSystemServiceGetPowerTick (0x7E3BC3E1AA549001)
    registerExport(0x7E3BC3E1AA549001ULL, "sceSystemServiceGetPowerTick",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            static auto start = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        });

    // sceSystemServiceHideSplashScreen (0x8C78FA6EEFEFCE01)
    registerExport(0x8C78FA6EEFEFCE01ULL, "sceSystemServiceHideSplashScreen",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            PROSPERO_LOG_INFO("HLE_SYS", "sceSystemServiceHideSplashScreen called -> Hiding PS5 boot splash");
            return SCE_OK;
        });

    // sceSystemServiceParamGetInt (0x367A43B1D56BE901)
    registerExport(0x367A43B1D56BE901ULL, "sceSystemServiceParamGetInt",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 param_id = static_cast<s32>(a1);
            vaddr_t out_val = a2;
            s32 default_val = 0;
            if (param_id == 0) default_val = 0; // Language: English
            if (out_val) {
                memory::MemoryManager::instance().write(out_val, &default_val, sizeof(s32));
            }
            PROSPERO_LOG_INFO("HLE_SYS", "sceSystemServiceParamGetInt (Param: " << param_id << ") -> " << default_val);
            return SCE_OK;
        });

    // sceSystemServiceParamGetString (0x8F56708BE09AC401)
    registerExport(0x8F56708BE09AC401ULL, "sceSystemServiceParamGetString",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            s32 param_id = static_cast<s32>(a1);
            vaddr_t out_buf = a2;
            size_t max_len = static_cast<size_t>(a3);
            const char* val = "PPSA01546"; // Default title ID
            if (out_buf && max_len > 0) {
                memory::MemoryManager::instance().write(out_buf, val, strlen(val) + 1);
            }
            PROSPERO_LOG_INFO("HLE_SYS", "sceSystemServiceParamGetString (Param: " << param_id << ") -> '" << val << "'");
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
