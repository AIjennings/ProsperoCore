#pragma once

#include <cstdint>
#include <cstddef>

namespace prospero {

// Basic integer types
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

// Memory addresses
using vaddr_t = uint64_t; // 64-bit Virtual Address
using paddr_t = uint64_t; // 64-bit Physical Address (Direct Memory)
using off_t   = int64_t;  // Offset

// Prospero Virtual Memory Boundaries
constexpr vaddr_t PROSPERO_PAGE_SIZE            = 0x4000;         // 16 KiB default page
constexpr vaddr_t PROSPERO_USER_ADDR_MIN        = 0x000000400000ULL;
constexpr vaddr_t PROSPERO_USER_ADDR_MAX        = 0x00007FFFFFFFFFFFULL;
constexpr vaddr_t PROSPERO_DIRECT_MEM_START     = 0x000080000000ULL; // 2 GiB onwards for Direct Memory pools
constexpr u64     PROSPERO_TOTAL_SYSTEM_MEM     = 16ULL * 1024ULL * 1024ULL * 1024ULL; // 16 GB GDDR6

// Common Sony SCE Result Codes
constexpr s32 SCE_OK                             = 0;
constexpr s32 SCE_KERNEL_ERROR_EPERM             = 1;
constexpr s32 SCE_KERNEL_ERROR_ENOENT            = 2;
constexpr s32 SCE_KERNEL_ERROR_ESRCH             = 3;
constexpr s32 SCE_KERNEL_ERROR_EINTR             = 4;
constexpr s32 SCE_KERNEL_ERROR_EIO               = 5;
constexpr s32 SCE_KERNEL_ERROR_ENXIO             = 6;
constexpr s32 SCE_KERNEL_ERROR_E2BIG             = 7;
constexpr s32 SCE_KERNEL_ERROR_ENOEXEC           = 8;
constexpr s32 SCE_KERNEL_ERROR_EBADF             = 9;
constexpr s32 SCE_KERNEL_ERROR_ECHILD            = 10;
constexpr s32 SCE_KERNEL_ERROR_EDEADLK           = 11;
constexpr s32 SCE_KERNEL_ERROR_ENOMEM            = 12;
constexpr s32 SCE_KERNEL_ERROR_EACCES            = 13;
constexpr s32 SCE_KERNEL_ERROR_EFAULT            = 14;
constexpr s32 SCE_KERNEL_ERROR_EBUSY             = 16;
constexpr s32 SCE_KERNEL_ERROR_EEXIST            = 17;
constexpr s32 SCE_KERNEL_ERROR_EXDEV             = 18;
constexpr s32 SCE_KERNEL_ERROR_ENODEV            = 19;
constexpr s32 SCE_KERNEL_ERROR_ENOTDIR           = 20;
constexpr s32 SCE_KERNEL_ERROR_EISDIR            = 21;
constexpr s32 SCE_KERNEL_ERROR_EINVAL            = 22;
constexpr s32 SCE_KERNEL_ERROR_ENFILE            = 23;
constexpr s32 SCE_KERNEL_ERROR_EMFILE            = 24;
constexpr s32 SCE_KERNEL_ERROR_ENOTTY            = 25;
constexpr s32 SCE_KERNEL_ERROR_ETXTBSY           = 26;
constexpr s32 SCE_KERNEL_ERROR_EFBIG             = 27;
constexpr s32 SCE_KERNEL_ERROR_ENOSPC            = 28;
constexpr s32 SCE_KERNEL_ERROR_ESPIPE            = 29;
constexpr s32 SCE_KERNEL_ERROR_EROFS             = 30;
constexpr s32 SCE_KERNEL_ERROR_EMLINK            = 31;
constexpr s32 SCE_KERNEL_ERROR_EPIPE             = 32;
constexpr s32 SCE_KERNEL_ERROR_ENOSYS            = 78;

// Memory Protection Flags (aligned with FreeBSD/Prospero)
constexpr u32 PROSPERO_PROT_NONE                 = 0x00;
constexpr u32 PROSPERO_PROT_READ                 = 0x01;
constexpr u32 PROSPERO_PROT_WRITE                = 0x02;
constexpr u32 PROSPERO_PROT_EXEC                 = 0x04;
constexpr u32 PROSPERO_PROT_ALL                  = (PROSPERO_PROT_READ | PROSPERO_PROT_WRITE | PROSPERO_PROT_EXEC);

// Memory Map Flags
constexpr u32 PROSPERO_MAP_SHARED                = 0x0001;
constexpr u32 PROSPERO_MAP_PRIVATE               = 0x0002;
constexpr u32 PROSPERO_MAP_FIXED                 = 0x0010;
constexpr u32 PROSPERO_MAP_ANONYMOUS             = 0x1000;
constexpr u32 PROSPERO_MAP_STACK                 = 0x0400;

} // namespace prospero
