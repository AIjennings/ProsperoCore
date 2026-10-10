#pragma once

#include "prospero/common/types.h"
#include "prospero/kernel/syscalls.h"
#include "prospero/kernel/abi_bridge.h"
#include <functional>
#include <unordered_map>
#include <string>

namespace prospero::kernel {

using SyscallHandler = std::function<u64(CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)>;

class SyscallDispatcher {
public:
    static SyscallDispatcher& instance() {
        static SyscallDispatcher s_instance;
        return s_instance;
    }

    bool init();
    void registerSyscall(u32 syscall_no, const std::string& name, SyscallHandler handler);
    u64 dispatch(u32 syscall_no, CpuContext& ctx, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);

    void dumpStats() const;

    // Virtual File System (VFS)
    void mount(const std::string& guest_prefix, const std::string& host_path);
    std::string resolvePath(const std::string& guest_path) const;

private:
    SyscallDispatcher() = default;

    struct SyscallEntry {
        std::string name;
        SyscallHandler handler;
        u64 call_count{0};
    };

    std::unordered_map<u32, SyscallEntry> m_syscalls;
    std::unordered_map<std::string, std::string> m_mounts;
    std::unordered_map<int, void*> m_open_files;
    int m_next_fd{10};
};

} // namespace prospero::kernel
