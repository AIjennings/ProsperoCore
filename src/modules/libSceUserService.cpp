#include "prospero/modules/libSceUserService.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <cstring>

namespace prospero::modules {

LibSceUserService::LibSceUserService() : HleModule("libSceUserService.sprx") {}

bool LibSceUserService::init() {
    // sceUserServiceInitialize (0x981C5C2A6B3D1401)
    registerExport(nids::sceUserServiceInitialize, "sceUserServiceInitialize",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            PROSPERO_LOG_INFO("HLE_USER", "sceUserServiceInitialize called -> SCE_OK");
            return SCE_OK;
        });

    // sceUserServiceGetInitialUser (0x486B2F995D71C801)
    registerExport(nids::sceUserServiceGetInitialUser, "sceUserServiceGetInitialUser",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            vaddr_t out_user_id = a1;
            u32 primary_user_id = 0x10000001; // Primary active user ID
            if (out_user_id) {
                memory::MemoryManager::instance().write(out_user_id, &primary_user_id, sizeof(u32));
            }
            PROSPERO_LOG_INFO("HLE_USER", "sceUserServiceGetInitialUser -> UserID: 0x" << std::hex << primary_user_id << std::dec);
            return SCE_OK;
        });

    // sceUserServiceGetUserName (0x5E518EAC8BE10401)
    registerExport(nids::sceUserServiceGetUserName, "sceUserServiceGetUserName",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 user_id = static_cast<u32>(a1);
            vaddr_t out_name = a2;
            size_t max_len = static_cast<size_t>(a3);
            const char* username = "Player1";
            if (out_name && max_len > 0) {
                size_t copy_len = std::min(max_len - 1, strlen(username));
                memory::MemoryManager::instance().write(out_name, username, copy_len + 1);
            }
            PROSPERO_LOG_INFO("HLE_USER", "sceUserServiceGetUserName for UserID: 0x" << std::hex << user_id << " -> 'Player1'");
            return SCE_OK;
        });

    return true;
}

} // namespace prospero::modules
