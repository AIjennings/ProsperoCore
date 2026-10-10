#include "prospero/modules/module_manager.h"
#include "prospero/modules/libkernel.h"
#include "prospero/modules/libSceVideoOut.h"
#include "prospero/modules/libScePad.h"
#include "prospero/modules/libSceSysmodule.h"
#include "prospero/modules/libSceUserService.h"
#include "prospero/modules/libSceSystemService.h"
#include "prospero/modules/libSceAudioOut.h"
#include "prospero/common/logging.h"
#include <iostream>
#include <iomanip>

namespace prospero::modules {

bool ModuleManager::init() {
    PROSPERO_LOG_INFO("MODULE", "Initializing Prospero HLE System Modules...");

    auto kernel = std::make_shared<LibKernel>();
    kernel->init();
    registerModule(kernel);

    auto video = std::make_shared<LibSceVideoOut>();
    video->init();
    registerModule(video);

    auto pad = std::make_shared<LibScePad>();
    pad->init();
    registerModule(pad);

    auto sysmod = std::make_shared<LibSceSysmodule>();
    sysmod->init();
    registerModule(sysmod);

    auto user = std::make_shared<LibSceUserService>();
    user->init();
    registerModule(user);

    auto syssvc = std::make_shared<LibSceSystemService>();
    syssvc->init();
    registerModule(syssvc);

    auto audio = std::make_shared<LibSceAudioOut>();
    audio->init();
    registerModule(audio);

    PROSPERO_LOG_INFO("MODULE", "Registered " << m_modules.size() << " core HLE modules with " 
                      << m_nid_to_module.size() << " total exported NIDs");
    return true;
}

void ModuleManager::registerModule(std::shared_ptr<HleModule> module) {
    if (!module) return;
    m_modules[module->getName()] = module;
    for (const auto& [nid, exp] : module->getExports()) {
        m_nid_to_module[nid] = module;
    }
}

std::shared_ptr<HleModule> ModuleManager::getModule(const std::string& name) {
    auto it = m_modules.find(name);
    if (it != m_modules.end()) return it->second;
    return nullptr;
}

bool ModuleManager::hasNid(nid_t nid) const {
    return m_nid_to_module.count(nid) > 0;
}

std::string ModuleManager::resolveName(nid_t nid) const {
    auto it = m_nid_to_module.find(nid);
    if (it != m_nid_to_module.end()) {
        const auto* exp = it->second->findExport(nid);
        if (exp) return exp->name;
    }
    return NidDatabase::instance().getName(nid);
}

u64 ModuleManager::callNid(nid_t nid, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) {
    auto it = m_nid_to_module.find(nid);
    if (it == m_nid_to_module.end()) {
        std::string resolved_name = NidDatabase::instance().getName(nid);
        PROSPERO_LOG_WARN("HLE_STUB", "Unhandled NID: 0x" << std::hex << nid 
                          << " (" << resolved_name << ") [Args: 0x" << a1 << ", 0x" << a2 
                          << ", 0x" << a3 << ", 0x" << a4 << "] -> Returning SCE_OK fallback");
        return SCE_OK;
    }

    auto* exp = it->second->findExport(nid);
    if (!exp) return SCE_OK;

    exp->call_count++;
    PROSPERO_LOG_INFO("HLE_CALL", exp->name << "(0x" << std::hex << a1 << ", 0x" << a2 
                      << ", 0x" << a3 << ", 0x" << a4 << std::dec << ")");

    u64 result = exp->func(a1, a2, a3, a4, a5, a6);
    return result;
}

void ModuleManager::dumpModules() const {
    std::cout << "\n=======================================================\n";
    std::cout << "             REGISTERED PROSPERO HLE MODULES           \n";
    std::cout << "=======================================================\n";
    for (const auto& [name, mod] : m_modules) {
        std::cout << "  Module: " << std::setw(24) << std::left << name 
                  << " (" << mod->getExports().size() << " exports)\n";
        for (const auto& [nid, exp] : mod->getExports()) {
            std::cout << "    - [0x" << std::hex << std::setw(16) << std::setfill('0') << nid 
                      << "] " << std::left << std::setw(32) << std::setfill(' ') << exp.name 
                      << " (Calls: " << std::dec << exp.call_count << ")\n";
        }
    }
    std::cout << "=======================================================\n\n";
}

} // namespace prospero::modules
