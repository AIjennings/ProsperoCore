#pragma once

#include "prospero/common/types.h"

namespace prospero::kernel {

// Standard FreeBSD 12 & Prospero Custom Syscall Numbers
constexpr u32 SYS_exit                  = 1;
constexpr u32 SYS_fork                  = 2;
constexpr u32 SYS_read                  = 3;
constexpr u32 SYS_write                 = 4;
constexpr u32 SYS_open                  = 5;
constexpr u32 SYS_close                 = 6;
constexpr u32 SYS_wait4                 = 7;
constexpr u32 SYS_getpid                = 20;
constexpr u32 SYS_mprotect              = 74;
constexpr u32 SYS_mprotect_x            = 75;
constexpr u32 SYS_ioctl                 = 54;
constexpr u32 SYS_mmap                  = 477;
constexpr u32 SYS_munmap                = 478;

// Sony Prospero / Orbis Custom Syscalls (Discovered via Kernel Dumps)
constexpr u32 SYS_regmgr_call           = 532;
constexpr u32 SYS_jitshm_create         = 533;
constexpr u32 SYS_jitshm_alias          = 534;
constexpr u32 SYS_dynlib_dlsym          = 536;
constexpr u32 SYS_dynlib_get_list       = 537;
constexpr u32 SYS_dynlib_get_info       = 538;
constexpr u32 SYS_dynlib_load_prx       = 539;
constexpr u32 SYS_dynlib_unload_prx     = 540;
constexpr u32 SYS_dynlib_get_info_ex    = 542;
constexpr u32 SYS_dynlib_get_info2      = 543;
constexpr u32 SYS_dynlib_get_proc_param = 544;
constexpr u32 SYS_mmap_dmem             = 587;
constexpr u32 SYS_munmap_dmem           = 588;
constexpr u32 SYS_dynlib_load_prx_ex    = 594;
constexpr u32 SYS_namedobj_create       = 608;
constexpr u32 SYS_namedobj_delete       = 609;
constexpr u32 SYS_equeue_create         = 610;
constexpr u32 SYS_equeue_delete         = 611;
constexpr u32 SYS_equeue_wait           = 612;

} // namespace prospero::kernel
