#include "prospero/kernel/syscall_dispatcher.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <iostream>
#include <iomanip>
#include <cstdio>

namespace prospero::kernel {

void SyscallDispatcher::mount(const std::string& guest_prefix, const std::string& host_path) {
    m_mounts[guest_prefix] = host_path;
    PROSPERO_LOG_INFO("VFS", "Mounted '" << guest_prefix << "' -> '" << host_path << "'");
}

std::string SyscallDispatcher::resolvePath(const std::string& guest_path) const {
    for (const auto& [prefix, host] : m_mounts) {
        if (guest_path.rfind(prefix, 0) == 0) { // Starts with prefix
            std::string sub = guest_path.substr(prefix.length());
            if (!sub.empty() && sub.front() == '/') sub.erase(0, 1);
            return host + "/" + sub;
        }
    }
    return guest_path;
}

bool SyscallDispatcher::init() {
    PROSPERO_LOG_INFO("KERNEL", "Registering FreeBSD 12 / Prospero Syscall Handlers...");

    // Default mounts
    mount("/app0", "./app0");
    mount("/savedata0", "./savedata");
    mount("/data", "./data");

    // SYS_exit (1)
    registerSyscall(SYS_exit, "sys_exit", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        int status = static_cast<int>(a1);
        PROSPERO_LOG_INFO("KERNEL", "Process exited via sys_exit with status: " << status);
        return 0;
    });

    // SYS_read (3)
    registerSyscall(SYS_read, "sys_read", [this](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        int fd = static_cast<int>(a1);
        vaddr_t buf_addr = a2;
        size_t count = a3;

        char* buf = memory::MemoryManager::instance().getPointer<char>(buf_addr);
        if (!buf) return static_cast<u64>(-1);

        auto it = m_open_files.find(fd);
        if (it != m_open_files.end() && it->second) {
            FILE* f = static_cast<FILE*>(it->second);
            size_t bytes_read = fread(buf, 1, count, f);
            return bytes_read;
        }
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

    // SYS_open (5)
    registerSyscall(SYS_open, "sys_open", [this](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t path_addr = a1;
        int flags = static_cast<int>(a2);
        const char* guest_path = memory::MemoryManager::instance().getPointer<const char>(path_addr);
        if (!guest_path) return static_cast<u64>(SCE_KERNEL_ERROR_EFAULT);

        std::string resolved = resolvePath(guest_path);
        FILE* f = fopen(resolved.c_str(), (flags & 2) ? "r+b" : "rb");
        if (!f) {
            // Try fallback relative path
            f = fopen(guest_path, "rb");
        }

        if (!f) {
            PROSPERO_LOG_WARN("VFS", "sys_open failed: '" << guest_path << "' (Resolved: '" << resolved << "')");
            return static_cast<u64>(SCE_KERNEL_ERROR_ENOENT);
        }

        int fd = m_next_fd++;
        m_open_files[fd] = f;
        PROSPERO_LOG_INFO("VFS", "sys_open: '" << guest_path << "' -> FD=" << fd);
        return static_cast<u64>(fd);
    });

    // SYS_close (6)
    registerSyscall(SYS_close, "sys_close", [this](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        int fd = static_cast<int>(a1);
        auto it = m_open_files.find(fd);
        if (it != m_open_files.end() && it->second) {
            fclose(static_cast<FILE*>(it->second));
            m_open_files.erase(it);
        }
        return SCE_OK;
    });

    // SYS_getpid (20)
    registerSyscall(SYS_getpid, "sys_getpid", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        return 0x1000; // Simulated Prospero PID
    });

    // SYS_ioctl (54)
    registerSyscall(SYS_ioctl, "sys_ioctl", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        return SCE_OK;
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

    // SYS_dynlib_get_proc_param (544)
    registerSyscall(SYS_dynlib_get_proc_param, "sys_dynlib_get_proc_param", [](CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) -> u64 {
        vaddr_t out_param = a1;
        vaddr_t out_size = a2;
        static u64 dummy_params[4] = {0x1000, 0x400000, 0, 0};
        if (out_param) {
            memory::MemoryManager::instance().write(out_param, dummy_params, sizeof(dummy_params));
        }
        if (out_size) {
            u64 sz = sizeof(dummy_params);
            memory::MemoryManager::instance().write(out_size, &sz, sizeof(sz));
        }
        return SCE_OK;
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
