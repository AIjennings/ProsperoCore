#pragma once

#include "prospero/common/types.h"
#include "prospero/common/nid.h"
#include "prospero/loader/elf.h"
#include <string>
#include <vector>
#include <memory>

namespace prospero::loader {

struct LoadedSegment {
    u32 type;
    u32 flags;
    vaddr_t vaddr;
    u64 mem_size;
    u64 file_offset;
    u64 file_size;
    std::vector<u8> data;
};

struct ModuleImport {
    std::string module_name;
    std::string library_name;
    nid_t nid;
    std::string symbol_name;
};

struct ModuleExport {
    std::string module_name;
    std::string library_name;
    nid_t nid;
    vaddr_t address;
    std::string symbol_name;
};

class ElfLoader {
public:
    ElfLoader();
    ~ElfLoader();

    bool loadFromFile(const std::string& path);
    bool loadFromMemory(const std::vector<u8>& raw_data);

    vaddr_t getEntryPoint() const { return m_entry_point; }
    vaddr_t getBaseAddress() const { return m_base_address; }
    const std::vector<LoadedSegment>& getSegments() const { return m_segments; }
    const std::vector<std::string>& getNeededModules() const { return m_needed_modules; }
    const std::vector<ModuleImport>& getImports() const { return m_imports; }
    const std::vector<ModuleExport>& getExports() const { return m_exports; }
    const std::string& getModuleName() const { return m_module_name; }
    const std::string& getFingerprint() const { return m_fingerprint; }

    void dumpInfo() const;

private:
    bool parseElf(const u8* data, size_t size);
    bool parseSelf(const u8* data, size_t size, std::vector<u8>& out_elf);
    bool parseDynamic(const u8* elf_base, size_t elf_size);

    std::vector<u8> m_elf_data;
    vaddr_t m_entry_point{0};
    vaddr_t m_base_address{0};
    std::string m_module_name{"unknown"};
    std::string m_fingerprint;
    std::vector<LoadedSegment> m_segments;
    std::vector<std::string> m_needed_modules;
    std::vector<ModuleImport> m_imports;
    std::vector<ModuleExport> m_exports;
};

} // namespace prospero::loader
