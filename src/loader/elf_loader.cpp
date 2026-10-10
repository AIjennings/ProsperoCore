#include "prospero/loader/elf_loader.h"
#include "prospero/loader/self.h"
#include "prospero/common/logging.h"
#include "prospero/common/nid.h"
#include <fstream>
#include <cstring>
#include <iomanip>
#include <algorithm>

namespace prospero::loader {

ElfLoader::ElfLoader() = default;
ElfLoader::~ElfLoader() = default;

bool ElfLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        PROSPERO_LOG_ERROR("LOADER", "Failed to open file: " << path);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        PROSPERO_LOG_ERROR("LOADER", "Failed to read binary data from: " << path);
        return false;
    }

    return loadFromMemory(buffer);
}

bool ElfLoader::loadFromMemory(const std::vector<u8>& raw_data) {
    if (raw_data.size() < sizeof(elf::Elf64_Ehdr)) {
        PROSPERO_LOG_ERROR("LOADER", "File size too small for ELF header (" << raw_data.size() << " bytes)");
        return false;
    }

    // Check for SELF header
    u32 magic = *reinterpret_cast<const u32*>(raw_data.data());
    if (magic == self::SELF_MAGIC_FSELF || magic == self::SELF_MAGIC_SCE) {
        PROSPERO_LOG_INFO("LOADER", "Detected Sony SELF/FSELF signed container (Magic: 0x" 
            << std::hex << magic << std::dec << ")");
        std::vector<u8> decrypted_elf;
        if (!parseSelf(raw_data.data(), raw_data.size(), decrypted_elf)) {
            PROSPERO_LOG_ERROR("LOADER", "Failed to parse SELF container");
            return false;
        }
        m_elf_data = std::move(decrypted_elf);
    } else if (raw_data[0] == elf::ELFMAG0 && raw_data[1] == elf::ELFMAG1 &&
               raw_data[2] == elf::ELFMAG2 && raw_data[3] == elf::ELFMAG3) {
        PROSPERO_LOG_INFO("LOADER", "Detected raw decrypted 64-bit ELF binary");
        m_elf_data = raw_data;
    } else {
        PROSPERO_LOG_ERROR("LOADER", "Unknown file magic: 0x" << std::hex << magic);
        return false;
    }

    return parseElf(m_elf_data.data(), m_elf_data.size());
}

bool ElfLoader::parseSelf(const u8* data, size_t size, std::vector<u8>& out_elf) {
    if (size < sizeof(self::SelfHeader)) return false;

    const auto* hdr = reinterpret_cast<const self::SelfHeader*>(data);
    PROSPERO_LOG_INFO("LOADER", "SELF Entries: " << hdr->num_entries << ", Header Size: " << hdr->header_size);

    size_t entries_offset = sizeof(self::SelfHeader);
    const auto* entries = reinterpret_cast<const self::SelfEntry*>(data + entries_offset);

    // Look for ELF header segment in SELF entries
    u64 elf_offset = 0;
    u64 elf_size = 0;
    for (u16 i = 0; i < hdr->num_entries; ++i) {
        if (entries[i].offset + entries[i].file_size <= size) {
            // Check if this segment contains standard ELF magic
            const u8* seg = data + entries[i].offset;
            if (seg[0] == elf::ELFMAG0 && seg[1] == elf::ELFMAG1 &&
                seg[2] == elf::ELFMAG2 && seg[3] == elf::ELFMAG3) {
                elf_offset = entries[i].offset;
                elf_size = entries[i].file_size;
                break;
            }
        }
    }

    if (elf_offset == 0) {
        // Fallback: check if ELF follows immediately after the SELF header
        if (hdr->header_size < size) {
            const u8* candidate = data + hdr->header_size;
            if (candidate[0] == elf::ELFMAG0 && candidate[1] == elf::ELFMAG1 &&
                candidate[2] == elf::ELFMAG2 && candidate[3] == elf::ELFMAG3) {
                elf_offset = hdr->header_size;
                elf_size = size - hdr->header_size;
            }
        }
    }

    if (elf_offset == 0) {
        PROSPERO_LOG_ERROR("LOADER", "Could not locate unencrypted ELF inside SELF container");
        return false;
    }

    out_elf.assign(data + elf_offset, data + elf_offset + elf_size);
    PROSPERO_LOG_INFO("LOADER", "Extracted inner ELF payload (" << out_elf.size() << " bytes)");
    return true;
}

bool ElfLoader::parseElf(const u8* data, size_t size) {
    const auto* ehdr = reinterpret_cast<const elf::Elf64_Ehdr*>(data);

    if (ehdr->e_ident[4] != elf::ELFCLASS64) {
        PROSPERO_LOG_ERROR("LOADER", "Unsupported ELF class: not 64-bit");
        return false;
    }

    if (ehdr->e_machine != elf::EM_X86_64) {
        PROSPERO_LOG_ERROR("LOADER", "Unsupported machine architecture: expected x86-64");
        return false;
    }

    m_entry_point = ehdr->e_entry;
    PROSPERO_LOG_INFO("LOADER", "Entry Point: 0x" << std::hex << m_entry_point << std::dec);
    PROSPERO_LOG_INFO("LOADER", "Program Headers: " << ehdr->e_phnum << ", Section Headers: " << ehdr->e_shnum);

    if (ehdr->e_phoff + ehdr->e_phnum * sizeof(elf::Elf64_Phdr) > size) {
        PROSPERO_LOG_ERROR("LOADER", "Program headers exceed file bounds");
        return false;
    }

    const auto* phdrs = reinterpret_cast<const elf::Elf64_Phdr*>(data + ehdr->e_phoff);
    m_base_address = 0xFFFFFFFFFFFFFFFFULL;

    for (u16 i = 0; i < ehdr->e_phnum; ++i) {
        const auto& ph = phdrs[i];
        if (ph.p_type == elf::PT_LOAD || ph.p_type == elf::PT_SCE_DYNLIBDATA || ph.p_type == elf::PT_SCE_RELRO) {
            LoadedSegment seg;
            seg.type = ph.p_type;
            seg.flags = ph.p_flags;
            seg.vaddr = ph.p_vaddr;
            seg.mem_size = ph.p_memsz;
            seg.file_offset = ph.p_offset;
            seg.file_size = ph.p_filesz;

            if (ph.p_offset + ph.p_filesz <= size) {
                seg.data.assign(data + ph.p_offset, data + ph.p_offset + ph.p_filesz);
            }
            m_segments.push_back(std::move(seg));

            if (ph.p_vaddr < m_base_address) {
                m_base_address = ph.p_vaddr;
            }
        }
    }

    if (m_base_address == 0xFFFFFFFFFFFFFFFFULL) {
        m_base_address = 0;
    }

    PROSPERO_LOG_INFO("LOADER", "Base Address: 0x" << std::hex << m_base_address << std::dec);

    parseDynamic(data, size);

    return true;
}

bool ElfLoader::parseDynamic(const u8* elf_base, size_t elf_size) {
    const auto* ehdr = reinterpret_cast<const elf::Elf64_Ehdr*>(elf_base);
    const auto* phdrs = reinterpret_cast<const elf::Elf64_Phdr*>(elf_base + ehdr->e_phoff);

    const elf::Elf64_Phdr* dyn_phdr = nullptr;
    const elf::Elf64_Phdr* dynlib_data_phdr = nullptr;

    for (u16 i = 0; i < ehdr->e_phnum; ++i) {
        if (phdrs[i].p_type == elf::PT_DYNAMIC) {
            dyn_phdr = &phdrs[i];
        } else if (phdrs[i].p_type == elf::PT_SCE_DYNLIBDATA) {
            dynlib_data_phdr = &phdrs[i];
        }
    }

    if (!dyn_phdr) {
        PROSPERO_LOG_WARN("LOADER", "No PT_DYNAMIC segment found in binary");
        return false;
    }

    const auto* dyn_entries = reinterpret_cast<const elf::Elf64_Dyn*>(elf_base + dyn_phdr->p_offset);
    size_t dyn_count = dyn_phdr->p_filesz / sizeof(elf::Elf64_Dyn);

    u64 strtab_offset = 0;
    u64 strsz = 0;
    u64 symtab_offset = 0;
    u64 syment = sizeof(elf::Elf64_Sym);
    u64 symtab_sz = 0;

    std::vector<u64> needed_str_indices;

    for (size_t i = 0; i < dyn_count; ++i) {
        const auto& d = dyn_entries[i];
        if (d.d_tag == elf::DT_NULL) break;

        switch (d.d_tag) {
            case elf::DT_SCE_STRTAB:
            case elf::DT_STRTAB:
                strtab_offset = d.d_un.d_ptr;
                break;
            case elf::DT_SCE_STRSZ:
            case elf::DT_STRSZ:
                strsz = d.d_un.d_val;
                break;
            case elf::DT_SCE_SYMTAB:
            case elf::DT_SYMTAB:
                symtab_offset = d.d_un.d_ptr;
                break;
            case elf::DT_SCE_SYMENT:
            case elf::DT_SYMENT:
                syment = d.d_un.d_val;
                break;
            case elf::DT_SCE_SYMTABSZ:
                symtab_sz = d.d_un.d_val;
                break;
            case elf::DT_SCE_NEEDED_MODULE:
            case elf::DT_NEEDED:
                needed_str_indices.push_back(d.d_un.d_val);
                break;
            default:
                break;
        }
    }

    // Adjust offsets if in PT_SCE_DYNLIBDATA
    const char* strtab = nullptr;
    if (strtab_offset != 0) {
        if (dynlib_data_phdr && strtab_offset < dynlib_data_phdr->p_filesz) {
            strtab = reinterpret_cast<const char*>(elf_base + dynlib_data_phdr->p_offset + strtab_offset);
        } else if (strtab_offset < elf_size) {
            strtab = reinterpret_cast<const char*>(elf_base + strtab_offset);
        }
    }

    if (strtab) {
        for (u64 idx : needed_str_indices) {
            if (idx < strsz || strsz == 0) {
                std::string mod_name = strtab + idx;
                m_needed_modules.push_back(mod_name);
                PROSPERO_LOG_INFO("LOADER", "Required Module: " << mod_name);
            }
        }
    }

    return true;
}

void ElfLoader::dumpInfo() const {
    std::cout << "\n=======================================================\n";
    std::cout << "          PROSPERO SCE ELF64 BINARY INFO               \n";
    std::cout << "=======================================================\n";
    std::cout << "  Module Name:   " << m_module_name << "\n";
    std::cout << "  Entry Point:   0x" << std::hex << m_entry_point << "\n";
    std::cout << "  Base Address:  0x" << m_base_address << std::dec << "\n";
    std::cout << "  Segments:      " << m_segments.size() << "\n\n";

    std::cout << "  [Segments]\n";
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& seg = m_segments[i];
        std::string flags;
        if (seg.flags & elf::PF_R) flags += "R";
        if (seg.flags & elf::PF_W) flags += "W";
        if (seg.flags & elf::PF_X) flags += "X";

        std::cout << "   - #" << i << ": VAddr 0x" << std::hex << seg.vaddr 
                  << " | MemSize: 0x" << seg.mem_size 
                  << " | FileSize: 0x" << seg.file_size
                  << " | Flags: [" << flags << "]" << std::dec << "\n";
    }

    if (!m_needed_modules.empty()) {
        std::cout << "\n  [Needed Dynamic Modules (SPRX)]\n";
        for (const auto& mod : m_needed_modules) {
            std::cout << "   - " << mod << "\n";
        }
    }
    std::cout << "=======================================================\n\n";
}

} // namespace prospero::loader
