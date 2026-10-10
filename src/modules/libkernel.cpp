#include "prospero/modules/libkernel.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <chrono>
#include <thread>

namespace prospero::modules {

LibKernel::LibKernel() : HleModule("libkernel.sprx") {}

bool LibKernel::init() {
    registerMemoryExports();
    registerThreadExports();
    registerEqueueExports();
    registerTimeExports();
    return true;
}

void LibKernel::registerMemoryExports() {
    // sceKernelAllocateDirectMemory (0x486B2F995D71C8A0)
    registerExport(nids::sceKernelAllocateDirectMemory, "sceKernelAllocateDirectMemory",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            off_t start = static_cast<off_t>(a1);
            off_t end = static_cast<off_t>(a2);
            size_t len = static_cast<size_t>(a3);
            size_t align = static_cast<size_t>(a4);
            s32 type = static_cast<s32>(a5);
            vaddr_t out_paddr_addr = a6;

            off_t paddr = 0;
            s32 ret = memory::MemoryManager::instance().allocateDirectMemory(start, end, len, align, type, &paddr);
            if (ret == SCE_OK && out_paddr_addr != 0) {
                memory::MemoryManager::instance().write(out_paddr_addr, &paddr, sizeof(off_t));
            }
            return static_cast<u64>(ret);
        });

    // sceKernelMapDirectMemory (0x05E518EAC8BE104B)
    registerExport(nids::sceKernelMapDirectMemory, "sceKernelMapDirectMemory",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            vaddr_t out_addr_ptr = a1;
            size_t len = static_cast<size_t>(a2);
            s32 prot = static_cast<s32>(a3);
            s32 flags = static_cast<s32>(a4);
            off_t paddr = static_cast<off_t>(a5);
            size_t align = static_cast<size_t>(a6);

            void* mapped_addr = nullptr;
            s32 ret = memory::MemoryManager::instance().mapDirectMemory(&mapped_addr, len, prot, flags, paddr, align);
            if (ret == SCE_OK && out_addr_ptr != 0) {
                memory::MemoryManager::instance().write(out_addr_ptr, &mapped_addr, sizeof(void*));
            }
            return static_cast<u64>(ret);
        });

    // sceKernelReleaseDirectMemory (0x2A19741B26E33B76)
    registerExport(nids::sceKernelReleaseDirectMemory, "sceKernelReleaseDirectMemory",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            off_t paddr = static_cast<off_t>(a1);
            size_t len = static_cast<size_t>(a2);
            return static_cast<u64>(memory::MemoryManager::instance().releaseDirectMemory(paddr, len));
        });

    // sceKernelGetDirectMemorySize (0x8C78FA6EEFEFCEAE)
    registerExport(nids::sceKernelGetDirectMemorySize, "sceKernelGetDirectMemorySize",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            return PROSPERO_TOTAL_SYSTEM_MEM;
        });
}

void LibKernel::registerThreadExports() {
    // scePthreadCreate (0x367A43B1D56BE9C8)
    registerExport(nids::scePthreadCreate, "scePthreadCreate",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            vaddr_t thread_id_ptr = a1;
            vaddr_t attr = a2;
            vaddr_t entry_func = a3;
            vaddr_t arg = a4;

            static u32 s_next_thread_id = 0x100;
            u32 tid = ++s_next_thread_id;

            PROSPERO_LOG_INFO("HLE_KERNEL", "scePthreadCreate: ThreadID=" << tid 
                              << " Entry=0x" << std::hex << entry_func << std::dec);

            if (thread_id_ptr) {
                memory::MemoryManager::instance().write(thread_id_ptr, &tid, sizeof(u32));
            }
            return SCE_OK;
        });

    // scePthreadExit (0x49E1066C1E44917B)
    registerExport(nids::scePthreadExit, "scePthreadExit",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            PROSPERO_LOG_INFO("HLE_KERNEL", "scePthreadExit called");
            return SCE_OK;
        });

    // scePthreadJoin (0x981C5C2A6B3D14F0)
    registerExport(nids::scePthreadJoin, "scePthreadJoin",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 tid = static_cast<u32>(a1);
            PROSPERO_LOG_INFO("HLE_KERNEL", "scePthreadJoin called on ThreadID=" << tid);
            return SCE_OK;
        });
}

void LibKernel::registerEqueueExports() {
    static u32 s_next_equeue_id = 1;

    // sceKernelCreateEqueue (0x8F56708BE09AC49F)
    registerExport(nids::sceKernelCreateEqueue, "sceKernelCreateEqueue",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            vaddr_t eq_ptr = a1;
            vaddr_t name_addr = a2;

            u32 eq_id = s_next_equeue_id++;
            const char* name = memory::MemoryManager::instance().getPointer<const char>(name_addr);

            PROSPERO_LOG_INFO("HLE_KERNEL", "sceKernelCreateEqueue: ID=" << eq_id 
                              << " Name=" << (name ? name : "unnamed"));

            if (eq_ptr) {
                memory::MemoryManager::instance().write(eq_ptr, &eq_id, sizeof(u32));
            }
            return SCE_OK;
        });

    // sceKernelDeleteEqueue (0x6E4F181BA9AC1234)
    registerExport(nids::sceKernelDeleteEqueue, "sceKernelDeleteEqueue",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 eq_id = static_cast<u32>(a1);
            PROSPERO_LOG_INFO("HLE_KERNEL", "sceKernelDeleteEqueue: ID=" << eq_id);
            return SCE_OK;
        });

    // sceKernelAddUserEvent (0xA26F56A18D905101)
    registerExport(nids::sceKernelAddUserEvent, "sceKernelAddUserEvent",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            return SCE_OK;
        });

    // sceKernelWaitEqueue (0x1290FACDEB45198B)
    registerExport(nids::sceKernelWaitEqueue, "sceKernelWaitEqueue",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            return SCE_OK;
        });
}

void LibKernel::registerTimeExports() {
    // sceKernelUsleep (0x5D48AEF028C1A80F)
    registerExport(nids::sceKernelUsleep, "sceKernelUsleep",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            u32 microseconds = static_cast<u32>(a1);
            std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
            return SCE_OK;
        });

    // sceKernelGetProcessTime (0x7E3BC3E1AA54901B)
    registerExport(nids::sceKernelGetProcessTime, "sceKernelGetProcessTime",
        [](u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
            static auto s_start = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - s_start).count();
        });
}

} // namespace prospero::modules
