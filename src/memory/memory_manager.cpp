#include "prospero/memory/memory_manager.h"
#include "prospero/common/logging.h"
#include <windows.h>
#include <algorithm>
#include <iomanip>
#include <iostream>

namespace prospero::memory {

MemoryManager::~MemoryManager() {
    shutdown();
}

bool MemoryManager::init() {
    PROSPERO_LOG_INFO("MEMORY", "Initializing Prospero 48-bit Virtual Memory Subsystem...");
    PROSPERO_LOG_INFO("MEMORY", "Guest Address Range: 0x" << std::hex << PROSPERO_USER_ADDR_MIN 
                      << " - 0x" << PROSPERO_USER_ADDR_MAX << std::dec);
    PROSPERO_LOG_INFO("MEMORY", "Total Simulated GDDR6 Memory: 16 GiB");
    return true;
}

void MemoryManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& reg : m_regions) {
        if (reg.host_ptr) {
            VirtualFree(reg.host_ptr, 0, MEM_RELEASE);
            reg.host_ptr = nullptr;
        }
    }
    m_regions.clear();
    m_direct_blocks.clear();
}

static DWORD toWindowsProtect(u32 prot) {
    if ((prot & PROSPERO_PROT_READ) && (prot & PROSPERO_PROT_WRITE) && (prot & PROSPERO_PROT_EXEC))
        return PAGE_EXECUTE_READWRITE;
    if ((prot & PROSPERO_PROT_READ) && (prot & PROSPERO_PROT_EXEC))
        return PAGE_EXECUTE_READ;
    if ((prot & PROSPERO_PROT_READ) && (prot & PROSPERO_PROT_WRITE))
        return PAGE_READWRITE;
    if (prot & PROSPERO_PROT_READ)
        return PAGE_READONLY;
    if (prot & PROSPERO_PROT_EXEC)
        return PAGE_EXECUTE;
    return PAGE_NOACCESS;
}

vaddr_t MemoryManager::allocateVirtual(vaddr_t preferred_addr, size_t size, u32 prot, const std::string& tag) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Align size to page boundary (16KB)
    size_t aligned_size = (size + PROSPERO_PAGE_SIZE - 1) & ~(PROSPERO_PAGE_SIZE - 1);
    vaddr_t guest_addr = preferred_addr;

    if (guest_addr == 0) {
        guest_addr = m_next_vaddr;
        m_next_vaddr += aligned_size;
    }

    DWORD win_prot = toWindowsProtect(prot);
    u8* host_mem = reinterpret_cast<u8*>(VirtualAlloc(nullptr, aligned_size, MEM_COMMIT | MEM_RESERVE, win_prot));

    if (!host_mem) {
        PROSPERO_LOG_ERROR("MEMORY", "Failed to allocate host memory of size: 0x" << std::hex << aligned_size);
        return 0;
    }

    MemoryRegion reg;
    reg.guest_vaddr = guest_addr;
    reg.size = aligned_size;
    reg.host_ptr = host_mem;
    reg.protection = prot;
    reg.tag = tag.empty() ? "Anonymous" : tag;

    m_regions.push_back(reg);

    PROSPERO_LOG_INFO("MEMORY", "Allocated Virtual Region [0x" << std::hex << guest_addr 
                      << " - 0x" << (guest_addr + aligned_size) << "] Size: 0x" 
                      << aligned_size << " (" << reg.tag << ")" << std::dec);

    return guest_addr;
}

bool MemoryManager::freeVirtual(vaddr_t addr, size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_regions.begin(); it != m_regions.end(); ++it) {
        if (it->guest_vaddr == addr) {
            VirtualFree(it->host_ptr, 0, MEM_RELEASE);
            m_regions.erase(it);
            return true;
        }
    }
    return false;
}

bool MemoryManager::protectVirtual(vaddr_t addr, size_t size, u32 new_prot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& reg : m_regions) {
        if (addr >= reg.guest_vaddr && addr < reg.guest_vaddr + reg.size) {
            DWORD old_prot;
            DWORD win_prot = toWindowsProtect(new_prot);
            if (VirtualProtect(reg.host_ptr, reg.size, win_prot, &old_prot)) {
                reg.protection = new_prot;
                return true;
            }
        }
    }
    return false;
}

s32 MemoryManager::allocateDirectMemory(off_t search_start, off_t search_end, size_t length, size_t alignment, s32 type, off_t* out_paddr) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!out_paddr || length == 0) return SCE_KERNEL_ERROR_EINVAL;

    size_t align = alignment ? alignment : PROSPERO_PAGE_SIZE;
    size_t aligned_len = (length + align - 1) & ~(align - 1);

    paddr_t paddr = (m_next_direct_paddr + align - 1) & ~(align - 1);
    m_next_direct_paddr = paddr + aligned_len;

    // Allocate physical memory on host
    u8* host_mem = reinterpret_cast<u8*>(VirtualAlloc(nullptr, aligned_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!host_mem) return SCE_KERNEL_ERROR_ENOMEM;

    DirectMemoryBlock block;
    block.paddr = paddr;
    block.size = aligned_len;
    block.host_ptr = host_mem;
    block.is_allocated = true;

    m_direct_blocks[paddr] = block;
    *out_paddr = static_cast<off_t>(paddr);

    PROSPERO_LOG_INFO("MEMORY", "DirectMemory Allocated: PhysAddr=0x" << std::hex << paddr 
                      << " Size=0x" << aligned_len << std::dec);
    return SCE_OK;
}

s32 MemoryManager::releaseDirectMemory(off_t paddr, size_t length) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_direct_blocks.find(static_cast<paddr_t>(paddr));
    if (it == m_direct_blocks.end()) return SCE_KERNEL_ERROR_EINVAL;

    VirtualFree(it->second.host_ptr, 0, MEM_RELEASE);
    m_direct_blocks.erase(it);
    return SCE_OK;
}

s32 MemoryManager::mapDirectMemory(void** out_addr, size_t length, s32 prot, s32 flags, off_t paddr, size_t alignment) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!out_addr) return SCE_KERNEL_ERROR_EINVAL;

    auto it = m_direct_blocks.find(static_cast<paddr_t>(paddr));
    if (it == m_direct_blocks.end()) return SCE_KERNEL_ERROR_EINVAL;

    size_t align = alignment ? alignment : PROSPERO_PAGE_SIZE;
    size_t aligned_len = (length + align - 1) & ~(align - 1);

    vaddr_t guest_vaddr = (m_next_vaddr + align - 1) & ~(align - 1);
    m_next_vaddr = guest_vaddr + aligned_len;

    MemoryRegion reg;
    reg.guest_vaddr = guest_vaddr;
    reg.size = aligned_len;
    reg.host_ptr = it->second.host_ptr;
    reg.protection = prot;
    reg.is_direct = true;
    reg.direct_paddr = static_cast<paddr_t>(paddr);
    reg.tag = "DirectMemoryMapped";

    m_regions.push_back(reg);

    *out_addr = reinterpret_cast<void*>(guest_vaddr);
    PROSPERO_LOG_INFO("MEMORY", "DirectMemory Mapped: PhysAddr=0x" << std::hex << paddr 
                      << " -> GuestVAddr=0x" << guest_vaddr << " (Size: 0x" << aligned_len << ")" << std::dec);

    return SCE_OK;
}

bool MemoryManager::read(vaddr_t guest_addr, void* dest, size_t size) {
    void* ptr = getPointer(guest_addr);
    if (!ptr) return false;
    std::memcpy(dest, ptr, size);
    return true;
}

bool MemoryManager::write(vaddr_t guest_addr, const void* src, size_t size) {
    void* ptr = getPointer(guest_addr);
    if (!ptr) return false;
    std::memcpy(ptr, src, size);
    return true;
}

vaddr_t MemoryManager::setupUserStack(size_t stack_size) {
    vaddr_t stack_top = 0x00007FFFF0000000ULL;
    vaddr_t stack_base = stack_top - stack_size;
    allocateVirtual(stack_base, stack_size, PROSPERO_PROT_READ | PROSPERO_PROT_WRITE, "UserStack");
    return stack_top;
}

void MemoryManager::dumpMemoryMap() const {
    std::cout << "\n=======================================================\n";
    std::cout << "             PROSPERO VIRTUAL MEMORY MAP               \n";
    std::cout << "=======================================================\n";
    for (const auto& reg : m_regions) {
        std::string prot;
        prot += (reg.protection & PROSPERO_PROT_READ) ? "R" : "-";
        prot += (reg.protection & PROSPERO_PROT_WRITE) ? "W" : "-";
        prot += (reg.protection & PROSPERO_PROT_EXEC) ? "X" : "-";

        std::cout << " [0x" << std::hex << std::setw(12) << std::setfill('0') << reg.guest_vaddr 
                  << " - 0x" << std::setw(12) << std::setfill('0') << (reg.guest_vaddr + reg.size) 
                  << "] Size: 0x" << std::setw(8) << reg.size 
                  << " | Prot: [" << prot << "] | " << reg.tag;
        if (reg.is_direct) {
            std::cout << " (Direct PAddr: 0x" << reg.direct_paddr << ")";
        }
        std::cout << std::dec << "\n";
    }
    std::cout << "=======================================================\n\n";
}

} // namespace prospero::memory
