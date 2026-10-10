#include "prospero/modules/libSceSysmodule.h"
#include "prospero/common/logging.h"

namespace prospero::modules {

LibSceSysmodule::LibSceSysmodule() : HleModule("libSceSysmodule.sprx") {}

bool LibSceSysmodule::init() {
    // sceSysmoduleLoadModule (0x91CAE145B6C078A1)
    registerExport(nids::sceSysmoduleLoadModule, "sceSysmoduleLoadModule",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 moduleId = static_cast<u32>(a1);
            m_loaded_modules.insert(moduleId);
            PROSPERO_LOG_INFO("HLE_SYSMODULE", "sceSysmoduleLoadModule: ModuleId=0x" 
                              << std::hex << moduleId << std::dec << " loaded successfully");
            return SCE_OK;
        });

    // sceSysmoduleUnloadModule (0x86AC17B0925CF184)
    registerExport(nids::sceSysmoduleUnloadModule, "sceSysmoduleUnloadModule",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 moduleId = static_cast<u32>(a1);
            m_loaded_modules.erase(moduleId);
            PROSPERO_LOG_INFO("HLE_SYSMODULE", "sceSysmoduleUnloadModule: ModuleId=0x" 
                              << std::hex << moduleId << std::dec);
            return SCE_OK;
        });

    // sceSysmoduleIsLoaded (0x310A7456BCD98762)
    registerExport(nids::sceSysmoduleIsLoaded, "sceSysmoduleIsLoaded",
        [this](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 moduleId = static_cast<u32>(a1);
            return m_loaded_modules.count(moduleId) ? SCE_OK : SCE_KERNEL_ERROR_ENOENT;
        });

    return true;
}

} // namespace prospero::modules
