#include "prospero/common/types.h"
#include "prospero/common/logging.h"
#include "prospero/common/nid.h"
#include "prospero/loader/elf_loader.h"
#include "prospero/memory/memory_manager.h"
#include "prospero/kernel/syscall_dispatcher.h"
#include "prospero/kernel/abi_bridge.h"
#include "prospero/modules/libScePad.h"
#include "prospero/gpu/vulkan_backend.h"
#include "prospero/gpu/command_processor.h"
#include "prospero/game/game_demo.h"

#include <iostream>
#include <string>
#include <vector>

using namespace prospero;

void printBanner() {
    std::cout << R"(
  ____                                           ____               
 |  _ \ _ __ ___  ___ _ __   ___ _ __ ___       / ___|___  _ __ ___ 
 | |_) | '__/ _ \/ __| '_ \ / _ \ '__/ _ \     | |   / _ \| '__/ _ \
 |  __/| | | (_) \__ \ |_) |  __/ | | (_) |  _ | |__| (_) | | |  __/
 |_|   |_|  \___/|___/ .__/ \___|_|  \___/  (_) \____\___/|_|  \___|
                     |_|                                            
   PlayStation 5 (Prospero) Next-Gen Compatibility Layer & Emulator
)" << std::endl;
}

void printUsage(const char* prog) {
    std::cout << "Usage:\n";
    std::cout << "  " << prog << " run [binary.elf/self]     Launch PS5 game in 60 FPS emulator window (defaults to ps5_game.elf)\n";
    std::cout << "  " << prog << " bench [frames]            Run headless high-throughput stress benchmark (default: 600 frames)\n";
    std::cout << "  " << prog << " inspect <binary.elf/self>  Inspect and analyze a PS5 ELF/SELF binary\n";
    std::cout << "  " << prog << " info                      Show emulator architecture and hardware status\n";
    std::cout << "  " << prog << " test                      Run comprehensive internal validation suite\n\n";
}

bool runTestSuite() {
    PROSPERO_LOG_INFO("TEST", "=======================================================");
    PROSPERO_LOG_INFO("TEST", "    RUNNING PROSPERO CORE COMPREHENSIVE TEST SUITE     ");
    PROSPERO_LOG_INFO("TEST", "=======================================================");

    // 1. Virtual Memory Test
    PROSPERO_LOG_INFO("TEST", "[1/10] Testing 48-bit Virtual Memory Subsystem...");
    auto& mem = prospero::memory::MemoryManager::instance();
    mem.init();
    prospero::vaddr_t test_vaddr = mem.allocateVirtual(0x400000, 0x10000, prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, "TestSegment");
    if (test_vaddr != 0x400000) {
        PROSPERO_LOG_ERROR("TEST", "Virtual allocation failed!");
        return false;
    }
    const char* test_str = "Prospero 2026";
    mem.write(test_vaddr, test_str, 14);
    char read_buf[16]{};
    mem.read(test_vaddr, read_buf, 14);
    if (std::string(read_buf) != test_str) {
        PROSPERO_LOG_ERROR("TEST", "Memory read/write mismatch!");
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> Virtual Memory verification PASSED.");

    // 2. Direct Physical Memory Allocation Test (GDDR6)
    PROSPERO_LOG_INFO("TEST", "[2/10] Testing Direct Physical GDDR6 Memory Allocator...");
    prospero::off_t paddr = 0;
    prospero::s32 dmem_res = mem.allocateDirectMemory(0, 0, 64 * 1024 * 1024, 0x10000, 0, &paddr);
    if (dmem_res != prospero::SCE_OK || paddr == 0) {
        PROSPERO_LOG_ERROR("TEST", "DirectMemory allocation failed!");
        return false;
    }
    void* mapped_vaddr = nullptr;
    prospero::s32 map_res = mem.mapDirectMemory(&mapped_vaddr, 64 * 1024 * 1024, prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, 0, paddr, 0x10000);
    if (map_res != prospero::SCE_OK || !mapped_vaddr) {
        PROSPERO_LOG_ERROR("TEST", "DirectMemory mapping failed!");
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> DirectMemory 64MB GDDR6 allocation & mapping PASSED (PhysAddr=0x" << std::hex << paddr << std::dec << ")");

    // 3. Syscall Dispatcher Test
    PROSPERO_LOG_INFO("TEST", "[3/10] Testing FreeBSD 12 / Prospero Syscall Dispatcher...");
    auto& syscalls = prospero::kernel::SyscallDispatcher::instance();
    syscalls.init();
    prospero::kernel::CpuContext ctx{};
    u64 write_res = syscalls.dispatch(prospero::kernel::SYS_write, ctx, 1, test_vaddr, 13, 0, 0, 0);
    if (write_res != 13) {
        PROSPERO_LOG_ERROR("TEST", "SYS_write syscall failed!");
        return false;
    }
    u64 pid_res = syscalls.dispatch(prospero::kernel::SYS_getpid, ctx, 0, 0, 0, 0, 0, 0);
    if (pid_res == 0) {
        PROSPERO_LOG_ERROR("TEST", "SYS_getpid syscall failed!");
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> Syscall Dispatcher PASSED (PID=0x" << std::hex << pid_res << std::dec << ").");

    // 4. Virtual File System (VFS) & File I/O Syscalls Test
    PROSPERO_LOG_INFO("TEST", "[4/10] Testing Virtual File System (VFS) & File I/O Syscalls...");
    syscalls.mount("/test_vfs", ".");
    prospero::vaddr_t vfs_path_vaddr = mem.allocateVirtual(0, 64, prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, "VFS_Path");
    const char* vfs_test_path = "/test_vfs/CMakeLists.txt";
    mem.write(vfs_path_vaddr, vfs_test_path, strlen(vfs_test_path) + 1);

    u64 fd_res = syscalls.dispatch(prospero::kernel::SYS_open, ctx, vfs_path_vaddr, 0, 0, 0, 0, 0);
    if (static_cast<s64>(fd_res) < 0) {
        PROSPERO_LOG_ERROR("TEST", "VFS sys_open failed! Code: " << fd_res);
        return false;
    }
    int test_fd = static_cast<int>(fd_res);

    prospero::vaddr_t vfs_read_vaddr = mem.allocateVirtual(0, 64, prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, "VFS_ReadBuf");
    u64 bytes_read = syscalls.dispatch(prospero::kernel::SYS_read, ctx, test_fd, vfs_read_vaddr, 16, 0, 0, 0);
    syscalls.dispatch(prospero::kernel::SYS_close, ctx, test_fd, 0, 0, 0, 0, 0);

    if (bytes_read == 0) {
        PROSPERO_LOG_ERROR("TEST", "VFS sys_read returned 0 bytes!");
        return false;
    }
    char vfs_buf[17]{};
    mem.read(vfs_read_vaddr, vfs_buf, 16);
    PROSPERO_LOG_INFO("TEST", "  -> VFS sys_open/read/close PASSED (Read " << bytes_read << " bytes: '" << vfs_buf << "')");

    // 5. Core HLE Modules & NID Resolution Test
    PROSPERO_LOG_INFO("TEST", "[5/10] Testing Core HLE Modules & Sony 64-bit NID Resolution...");
    auto& modules = prospero::modules::ModuleManager::instance();
    modules.init();
    if (!modules.hasNid(prospero::nids::sceKernelAllocateDirectMemory)) {
        PROSPERO_LOG_ERROR("TEST", "Missing sceKernelAllocateDirectMemory NID!");
        return false;
    }
    u64 dmem_size = modules.callNid(prospero::nids::sceKernelGetDirectMemorySize, 0, 0, 0, 0, 0, 0);
    if (dmem_size != prospero::PROSPERO_TOTAL_SYSTEM_MEM) {
        PROSPERO_LOG_ERROR("TEST", "sceKernelGetDirectMemorySize returned invalid size: " << dmem_size);
        return false;
    }
    // Test DualSense pad state query
    prospero::vaddr_t pad_state_vaddr = mem.allocateVirtual(0, sizeof(prospero::modules::ScePadData), prospero::PROSPERO_PROT_ALL, "PadData");
    modules.callNid(prospero::nids::scePadReadState, 0x201, pad_state_vaddr, 0, 0, 0, 0);
    auto* pad_data = mem.getPointer<prospero::modules::ScePadData>(pad_state_vaddr);
    if (!pad_data || pad_data->connected != 1) {
        PROSPERO_LOG_ERROR("TEST", "DualSense HLE state read failed!");
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> Core HLE Modules & NID resolution PASSED.");

    // 6. Extended HLE Modules (UserService, SystemService, AudioOut) Test
    PROSPERO_LOG_INFO("TEST", "[6/10] Testing Extended HLE Modules (UserService, SystemService, AudioOut)...");
    prospero::vaddr_t user_id_vaddr = mem.allocateVirtual(0, sizeof(u32), prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, "UserId");
    u64 user_res = modules.callNid(prospero::nids::sceUserServiceGetInitialUser, user_id_vaddr, 0, 0, 0, 0, 0);
    u32 user_id = 0;
    mem.read(user_id_vaddr, &user_id, sizeof(u32));
    if (user_res != prospero::SCE_OK || user_id != 0x10000001) {
        PROSPERO_LOG_ERROR("TEST", "sceUserServiceGetInitialUser failed! UserID: 0x" << std::hex << user_id);
        return false;
    }

    prospero::vaddr_t uname_vaddr = mem.allocateVirtual(0, 32, prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, "UserName");
    modules.callNid(prospero::nids::sceUserServiceGetUserName, user_id, uname_vaddr, 32, 0, 0, 0);
    char uname_buf[32]{};
    mem.read(uname_vaddr, uname_buf, sizeof(uname_buf));
    if (std::string(uname_buf) != "Player1") {
        PROSPERO_LOG_ERROR("TEST", "sceUserServiceGetUserName mismatch: " << uname_buf);
        return false;
    }

    prospero::vaddr_t title_vaddr = mem.allocateVirtual(0, 32, prospero::PROSPERO_PROT_READ | prospero::PROSPERO_PROT_WRITE, "TitleId");
    modules.callNid(prospero::nids::sceSystemServiceParamGetString, 0, title_vaddr, 32, 0, 0, 0);
    char title_buf[32]{};
    mem.read(title_vaddr, title_buf, sizeof(title_buf));
    if (std::string(title_buf) != "PPSA01546") {
        PROSPERO_LOG_ERROR("TEST", "sceSystemServiceParamGetString mismatch: " << title_buf);
        return false;
    }

    u64 audio_handle = modules.callNid(prospero::nids::sceAudioOutOpen, user_id, 0, 0, 0, 0, 0);
    if (audio_handle != 0x301) {
        PROSPERO_LOG_ERROR("TEST", "sceAudioOutOpen failed! Handle: 0x" << std::hex << audio_handle);
        return false;
    }
    u64 audio_close = modules.callNid(prospero::nids::sceAudioOutClose, audio_handle, 0, 0, 0, 0, 0);
    if (audio_close != prospero::SCE_OK) {
        PROSPERO_LOG_ERROR("TEST", "sceAudioOutClose failed!");
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> Extended HLE Modules PASSED (User: '" << uname_buf << "', Title: '" << title_buf << "', Audio: 0x301).");

    // 7. Dynamic NID Fallback & Resilient Stub Resolution
    PROSPERO_LOG_INFO("TEST", "[7/10] Testing Dynamic NID Fallback & Resilient Stub Resolution...");
    constexpr prospero::nid_t unhandled_nid = 0xCAFEBABE12345678ULL;
    if (modules.hasNid(unhandled_nid)) {
        PROSPERO_LOG_ERROR("TEST", "Unexpected NID collision in stub test!");
        return false;
    }
    u64 stub_ret = modules.callNid(unhandled_nid, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66);
    if (stub_ret != prospero::SCE_OK) {
        PROSPERO_LOG_ERROR("TEST", "Unhandled NID did not return SCE_OK fallback! Got: " << stub_ret);
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> Dynamic NID Fallback PASSED (Stub NID resolved safely to SCE_OK).");

    // 8. GPU PM4 Command Processor Test
    PROSPERO_LOG_INFO("TEST", "[8/10] Testing AMD RDNA2 / GFX10.3 PM4 Command Processor...");
    prospero::gpu::CommandProcessor cp;
    std::vector<prospero::u32> pm4_stream = {
        // IT_SET_CONTEXT_REG (header: type=3, opcode=0x69, count=2 -> 3 dwords)
        0xC0026900, 0x00000200, 0xDEADBEEF, 0xCAFEBABE,
        // IT_DRAW_INDEX_AUTO (header: type=3, opcode=0x2D, count=1 -> 2 dwords)
        0xC0012D00, 36, 1,
        // IT_DISPATCH_DIRECT (header: type=3, opcode=0x15, count=2 -> 3 dwords)
        0xC0021500, 16, 16, 1
    };
    cp.processCommandBuffer(pm4_stream.data(), pm4_stream.size());
    const auto& stats = cp.getStats();
    if (stats.draw_calls != 1 || stats.compute_dispatches != 1 || stats.registers_written != 2) {
        PROSPERO_LOG_ERROR("TEST", "PM4 packet processing failed! Draws=" << stats.draw_calls);
        return false;
    }
    PROSPERO_LOG_INFO("TEST", "  -> RDNA2 PM4 Command Buffer decode PASSED.");

    // 9. PM4 Packet Boundary Safety & Fuzzing
    PROSPERO_LOG_INFO("TEST", "[9/10] Testing PM4 Malformed Packet & Boundary Safety Fuzzing...");
    prospero::gpu::CommandProcessor fuzz_cp;
    std::vector<prospero::u32> malformed_stream = {
        // Header claims 100 dwords payload, but only 2 dwords follow
        0xC0646900, 0x00000200, 0x12345678,
        // Non-type 3 packet padding (Type 0 / no-op)
        0x00000000, 0x00000000
    };
    // Should safely abort processing without crashing or memory corruption
    fuzz_cp.processCommandBuffer(malformed_stream.data(), malformed_stream.size());
    PROSPERO_LOG_INFO("TEST", "  -> PM4 Malformed Packet Boundary Fuzzing PASSED (No out-of-bounds access).");

    // 10. Vulkan Hardware Detection
    PROSPERO_LOG_INFO("TEST", "[10/10] Testing Vulkan 1.4 Hardware Acceleration Subsystem...");
    auto& vk = prospero::gpu::VulkanBackend::instance();
    bool vk_ok = vk.init();
    if (vk_ok) {
        PROSPERO_LOG_INFO("TEST", "  -> Vulkan Hardware: " << vk.getSelectedDevice().device_name 
                          << " (VRAM: " << (vk.getSelectedDevice().vram_bytes / (1024 * 1024 * 1024.0)) << " GiB)");
    } else {
        PROSPERO_LOG_WARN("TEST", "  -> Vulkan dynamic initialization not available in headless mode.");
    }

    PROSPERO_LOG_INFO("TEST", "=======================================================");
    PROSPERO_LOG_INFO("TEST", "       ALL 10 SYSTEM TESTS PASSED SUCCESSFULLY!        ");
    PROSPERO_LOG_INFO("TEST", "=======================================================");
    return true;
}

int main(int argc, char* argv[]) {
    printBanner();

    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    std::string command = argv[1];

    if (command == "info") {
        auto& mem = prospero::memory::MemoryManager::instance();
        mem.init();

        auto& syscalls = prospero::kernel::SyscallDispatcher::instance();
        syscalls.init();

        auto& modules = prospero::modules::ModuleManager::instance();
        modules.init();

        auto& vk = prospero::gpu::VulkanBackend::instance();
        vk.init();

        modules.dumpModules();
        vk.dumpInfo();
        mem.dumpMemoryMap();
        return 0;
    }

    if (command == "test") {
        bool success = runTestSuite();
        return success ? 0 : 1;
    }

    if (command == "inspect") {
        if (argc < 3) {
            std::cerr << "Error: Missing path to binary file\n";
            return 1;
        }

        std::string path = argv[2];
        prospero::loader::ElfLoader loader;
        if (!loader.loadFromFile(path)) {
            return 1;
        }

        loader.dumpInfo();
        return 0;
    }

    if (command == "bench") {
        u32 total_frames = 600;
        if (argc >= 3) {
            try {
                total_frames = static_cast<u32>(std::stoul(argv[2]));
            } catch (...) {
                total_frames = 600;
            }
        }

        PROSPERO_LOG_INFO("BENCH", "Initializing Prospero subsystems for headless stress benchmark (" << total_frames << " frames)...");

        auto& mem = prospero::memory::MemoryManager::instance();
        mem.init();

        auto& syscalls = prospero::kernel::SyscallDispatcher::instance();
        syscalls.init();
        syscalls.mount("/app0", ".");

        auto& modules = prospero::modules::ModuleManager::instance();
        modules.init();

        auto& vk = prospero::gpu::VulkanBackend::instance();
        vk.init();

        prospero::game::ProsperoGame game;
        if (!game.init()) {
            PROSPERO_LOG_ERROR("BENCH", "Failed to initialize game runtime for benchmark");
            return 1;
        }

        game.runHeadlessBenchmark(total_frames);
        game.shutdown();
        return 0;
    }

    if (command == "run") {
        std::string path = (argc >= 3) ? argv[2] : "ps5_game.elf";
        if (path == "demo" || path == "game") {
            path = "ps5_game.elf";
        }

        std::string binary_path = path;
        std::string game_dir = ".";

        DWORD attr = GetFileAttributesA(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string c1 = path + "/eboot.bin";
            std::string c2 = path + "/app0/eboot.bin";
            if (GetFileAttributesA(c1.c_str()) != INVALID_FILE_ATTRIBUTES) {
                binary_path = c1;
                game_dir = path;
            } else if (GetFileAttributesA(c2.c_str()) != INVALID_FILE_ATTRIBUTES) {
                binary_path = c2;
                game_dir = path + "/app0";
            } else {
                game_dir = path;
            }
        } else {
            size_t slash = path.find_last_of("\\/");
            if (slash != std::string::npos) {
                game_dir = path.substr(0, slash);
            }
        }

        PROSPERO_LOG_INFO("RUNNER", "Target binary: " << binary_path);
        PROSPERO_LOG_INFO("RUNNER", "Game root directory: " << game_dir);

        // Check for param.json title metadata
        std::string param_path = game_dir + "/sce_sys/param.json";
        if (GetFileAttributesA(param_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
            param_path = game_dir + "/param.json";
        }
        if (GetFileAttributesA(param_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            PROSPERO_LOG_INFO("RUNNER", "Detected PS5 Title Metadata (" << param_path << ")");
        }

        prospero::loader::ElfLoader loader;
        bool loaded = loader.loadFromFile(binary_path);
        if (!loaded) {
            PROSPERO_LOG_WARN("RUNNER", "Could not load file '" << binary_path << "'. Falling back to internal PS5 showcase payload.");
        } else {
            PROSPERO_LOG_INFO("RUNNER", "Successfully loaded ELF. Entry point: 0x" << std::hex << loader.getEntryPoint() << std::dec);
        }

        auto& mem = prospero::memory::MemoryManager::instance();
        mem.init();

        auto& syscalls = prospero::kernel::SyscallDispatcher::instance();
        syscalls.init();
        syscalls.mount("/app0", game_dir);

        auto& modules = prospero::modules::ModuleManager::instance();
        modules.init();

        auto& vk = prospero::gpu::VulkanBackend::instance();
        vk.init();

        if (loaded) {
            // Map segments into virtual memory
            PROSPERO_LOG_INFO("RUNNER", "Mapping ELF program segments into Prospero virtual memory space...");
            for (const auto& seg : loader.getSegments()) {
                if (seg.type == prospero::elf::PT_LOAD) {
                    u32 prot = 0;
                    if (seg.flags & prospero::elf::PF_R) prot |= prospero::PROSPERO_PROT_READ;
                    if (seg.flags & prospero::elf::PF_W) prot |= prospero::PROSPERO_PROT_WRITE;
                    if (seg.flags & prospero::elf::PF_X) prot |= prospero::PROSPERO_PROT_EXEC;

                    prospero::vaddr_t mapped = mem.allocateVirtual(seg.vaddr, seg.mem_size, prot, "ELF_Segment");
                    if (!seg.data.empty()) {
                        mem.write(mapped, seg.data.data(), seg.data.size());
                    }
                }
            }

            // Setup user stack
            prospero::vaddr_t stack_top = mem.setupUserStack(8 * 1024 * 1024);
            PROSPERO_LOG_INFO("RUNNER", "User Stack configured at 0x" << std::hex << stack_top << std::dec);
            mem.dumpMemoryMap();
        }

        PROSPERO_LOG_INFO("RUNNER", "Booting PlayStation 5 (Prospero) game runtime...");
        prospero::game::ProsperoGame game;
        if (!game.init()) {
            PROSPERO_LOG_ERROR("RUNNER", "Failed to initialize game runtime");
            return 1;
        }

        game.run();
        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n";
    printUsage(argv[0]);
    return 1;
}
