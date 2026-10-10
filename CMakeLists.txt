cmake_minimum_required(VERSION 3.16)
project(ProsperoCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if (MSVC)
    add_compile_options(/W4 /WX- /permissive- /utf-8 /EHsc /bigobj /wd4100 /wd4189)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core library
set(PROSPERO_CORE_SOURCES
    src/loader/elf_loader.cpp
    src/memory/memory_manager.cpp
    src/kernel/syscall_dispatcher.cpp
    src/kernel/abi_bridge.cpp
    src/modules/module_manager.cpp
    src/modules/libkernel.cpp
    src/modules/libSceVideoOut.cpp
    src/modules/libScePad.cpp
    src/modules/libSceSysmodule.cpp
    src/gpu/command_processor.cpp
    src/gpu/vulkan_backend.cpp
)

add_library(prospero_core STATIC ${PROSPERO_CORE_SOURCES})

# CLI Executable
add_executable(prospero_cli src/main.cpp)
target_link_libraries(prospero_cli PRIVATE prospero_core)

# Synthetic Payload Generator Tool
add_executable(generate_ps5_payload tests/generate_ps5_payload.cpp)
target_link_libraries(generate_ps5_payload PRIVATE prospero_core)
