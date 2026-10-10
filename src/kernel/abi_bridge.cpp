#include "prospero/kernel/abi_bridge.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <iostream>
#include <iomanip>

namespace prospero::kernel {

void CpuContext::dump() const {
    std::cout << "\n=======================================================\n";
    std::cout << "               PROSPERO CPU CONTEXT (x86-64)           \n";
    std::cout << "=======================================================\n";
    std::cout << "  RAX: 0x" << std::hex << std::setw(16) << std::setfill('0') << rax 
              << "  RBX: 0x" << std::setw(16) << std::setfill('0') << rbx << "\n";
    std::cout << "  RCX: 0x" << std::setw(16) << std::setfill('0') << rcx 
              << "  RDX: 0x" << std::setw(16) << std::setfill('0') << rdx << "\n";
    std::cout << "  RSI: 0x" << std::setw(16) << std::setfill('0') << rsi 
              << "  RDI: 0x" << std::setw(16) << std::setfill('0') << rdi << "\n";
    std::cout << "  RBP: 0x" << std::setw(16) << std::setfill('0') << rbp 
              << "  RSP: 0x" << std::setw(16) << std::setfill('0') << rsp << "\n";
    std::cout << "  R8 : 0x" << std::setw(16) << std::setfill('0') << r8 
              << "  R9 : 0x" << std::setw(16) << std::setfill('0') << r9 << "\n";
    std::cout << "  R10: 0x" << std::setw(16) << std::setfill('0') << r10 
              << "  R11: 0x" << std::setw(16) << std::setfill('0') << r11 << "\n";
    std::cout << "  R12: 0x" << std::setw(16) << std::setfill('0') << r12 
              << "  R13: 0x" << std::setw(16) << std::setfill('0') << r13 << "\n";
    std::cout << "  R14: 0x" << std::setw(16) << std::setfill('0') << r14 
              << "  R15: 0x" << std::setw(16) << std::setfill('0') << r15 << "\n";
    std::cout << "  RIP: 0x" << std::setw(16) << std::setfill('0') << rip 
              << "  FLG: 0x" << std::setw(16) << std::setfill('0') << rflags << "\n";
    std::cout << "  FS : 0x" << std::setw(16) << std::setfill('0') << fs_base 
              << "  GS : 0x" << std::setw(16) << std::setfill('0') << gs_base << std::dec << "\n";
    std::cout << "=======================================================\n\n";
}

u64 AbiBridge::callHostFromSystemV(void* host_func, u64 rdi, u64 rsi, u64 rdx, u64 rcx, u64 r8, u64 r9) {
    if (!host_func) return 0;
    using HostFn = u64 (*)(u64, u64, u64, u64, u64, u64);
    auto fn = reinterpret_cast<HostFn>(host_func);
    // On Windows x64, fn receives args in RCX, RDX, R8, R9, [rsp+0x28], [rsp+0x30]
    return fn(rdi, rsi, rdx, rcx, r8, r9);
}

u64 AbiBridge::callGuestSystemV(vaddr_t guest_func, u64 arg1, u64 arg2, u64 arg3, u64 arg4) {
    void* host_ptr = memory::MemoryManager::instance().getPointer(guest_func);
    if (!host_ptr) {
        PROSPERO_LOG_ERROR("ABI", "Cannot call unmapped guest address: 0x" << std::hex << guest_func << std::dec);
        return static_cast<u64>(SCE_KERNEL_ERROR_EFAULT);
    }

    PROSPERO_LOG_INFO("ABI", "Executing Guest System V function at 0x" << std::hex << guest_func << std::dec);

    using HostFn = u64 (*)(u64, u64, u64, u64);
    auto fn = reinterpret_cast<HostFn>(host_ptr);
    return fn(arg1, arg2, arg3, arg4);
}

} // namespace prospero::kernel
