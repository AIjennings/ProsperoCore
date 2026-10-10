#pragma once

#include "prospero/common/types.h"
#include <string>

namespace prospero::kernel {

struct CpuContext {
    u64 rax{0};
    u64 rbx{0};
    u64 rcx{0};
    u64 rdx{0};
    u64 rsi{0};
    u64 rdi{0};
    u64 rbp{0};
    u64 rsp{0};
    u64 r8{0};
    u64 r9{0};
    u64 r10{0};
    u64 r11{0};
    u64 r12{0};
    u64 r13{0};
    u64 r14{0};
    u64 r15{0};
    u64 rip{0};
    u64 rflags{0};

    // Prospero TLS / Thread segment
    u64 fs_base{0};
    u64 gs_base{0};

    void dump() const;
};

class AbiBridge {
public:
    // Convert System V arguments to Microsoft x64 call
    static u64 callHostFromSystemV(void* host_func, u64 rdi, u64 rsi, u64 rdx, u64 rcx, u64 r8, u64 r9);

    // Call a guest function expecting System V AMD64 ABI from Windows host
    static u64 callGuestSystemV(vaddr_t guest_func, u64 arg1 = 0, u64 arg2 = 0, u64 arg3 = 0, u64 arg4 = 0);
};

} // namespace prospero::kernel
