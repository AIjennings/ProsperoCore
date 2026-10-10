#include "prospero/kernel/syscall_dispatcher.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <iostream>
#include <iomanip>

namespace prospero::kernel {

bool SyscallDispatcher::init() {
    PROSPERO_LOG_INFO("KERNEL", "Registering FreeBSD 12 / Prospero Syscall Handlers...");

    // SYS_exit (1)
    registerSyscall(SYS_exit, "sys_exit", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        int status = static_cast<int>(a1);
        PROSPERO_LOG_INFO("KERNEL", "Process exited via sys_exit with status: " << status);
        return 0;
    });

    // SYS_write (4)
    registerSyscall(SYS_write, "sys_write", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        int fd = static_cast<int>(a1);
        vaddr_t buf_addr = a2;
        size_t count = a3;

        const char* buf = memory::MemoryManager::instance().getPointer<const char>(buf_addr);
        if (!buf) return static_cast<u64>(-1);

        if (fd == 1 || fd == 2) {
            std::string str(buf, count);
            PROSPERO_LOG_INFO("GUEST_STDOUT", str);
            return count;
        }
        return count;
    });

    // SYS_mmap (477)
    registerSyscall(SYS_mmap, "sys_mmap", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t addr = a1;
        size_t len = a2;
        u32 prot = static_cast<u32>(a3);
        u32 flags = static_cast<u32>(a4);

        vaddr_t allocated = memory::MemoryManager::instance().allocateVirtual(addr, len, prot, "sys_mmap");
        return allocated;
    });

    // SYS_munmap (478)
    registerSyscall(SYS_munmap, "sys_munmap", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t addr = a1;
        size_t len = a2;
        bool res = memory::MemoryManager::instance().freeVirtual(addr, len);
        return res ? 0 : static_cast<u64>(SCE_KERNEL_ERROR_EINVAL);
    });

    // SYS_mprotect (74)
    registerSyscall(SYS_mprotect, "sys_mprotect", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t addr = a1;
        size_t len = a2;
        u32 prot = static_cast<u32>(a3);
        bool res = memory::MemoryManager::instance().protectVirtual(addr, len, prot);
        return res ? 0 : static_cast<u64>(SCE_KERNEL_ERROR_EINVAL);
    });

    // SYS_dynlib_load_prx (539)
    registerSyscall(SYS_dynlib_load_prx, "sys_dynlib_load_prx", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t name_addr = a1;
        const char* name = memory::MemoryManager::instance().getPointer<const char>(name_addr);
        PROSPERO_LOG_INFO("KERNEL", "sys_dynlib_load_prx: " << (name ? name : "<null>"));
        // Return simulated module handle
        return 0x2001;
    });

    // SYS_mmap_dmem (587)
    registerSyscall(SYS_mmap_dmem, "sys_mmap_dmem", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t out_addr_ptr = a1;
        size_t len = a2;
        u32 prot = static_cast<u32>(a3);
        u32 flags = static_cast<u32>(a4);
        off_t paddr = static_cast<off_t>(a5);

        void* mapped_addr = nullptr;
        s32 ret = memory::MemoryManager::instance().mapDirectMemory(&mapped_addr, len, prot, flags, paddr, 0);
        if (ret == SCE_OK && out_addr_ptr != 0) {
            memory::MemoryManager::instance().write(out_addr_ptr, &mapped_addr, sizeof(void*));
        }
        return ret;
    });

    // SYS_regmgr_call (532)
    registerSyscall(SYS_regmgr_call, "sys_regmgr_call", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        PROSPERO_LOG_INFO("KERNEL", "sys_regmgr_call (Stubbed)");
        return SCE_OK;
    });

    return true;
}

void SyscallDispatcher::registerSyscall(u32 syscall_no, const std::string& name, SyscallHandler handler) {
    m_syscalls[syscall_no] = {name, std::move(handler), 0};
}

u64 SyscallDispatcher::dispatch(u32 syscall_no, CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) {
    auto it = m_syscalls.find(syscall_no);
    if (it == m_syscalls.end()) {
        PROSPERO_LOG_WARN("SYSCALL", "Unimplemented Syscall #" << syscall_no 
                          << " (Args: 0x" << std::hex << a1 << ", 0x" << a2 << ", 0x" << a3 << std::dec << ")");
        return static_cast<u64>(SCE_KERNEL_ERROR_ENOSYS);
    }

    it->second.call_count++;
    PROSPERO_LOG_SYSCALL("SYSCALL", it->second.name << "(0x" << std::hex << a1 
                         << ", 0x" << a2 << ", 0x" << a3 << ", 0x" << a4 << std::dec << ")");

    u64 result = it->second.handler(ctx, a1, a2, a3, a4, a5, a6);
    PROSPERO_LOG_SYSCALL("SYSCALL", it->second.name << " returned 0x" << std::hex << result << std::dec);
    return result;
}

void SyscallDispatcher::dumpStats() const {
    std::cout << "\n=======================================================\n";
    std::cout << "             PROSPERO SYSCALL CALL STATS               \n";
    std::cout << "=======================================================\n";
    for (const auto& [num, entry] : m_syscalls) {
        if (entry.call_count > 0) {
            std::cout << "  Syscall #" << std::setw(3) << num << " [" << std::setw(22) << std::left 
                      << entry.name << "] : " << std::right << entry.call_count << " calls\n";
        }
    }
    std::cout << "=======================================================\n\n";
}

} // namespace prospero::kernel
