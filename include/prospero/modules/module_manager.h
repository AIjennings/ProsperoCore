#pragma once

#include "prospero/common/types.h"
#include "prospero/common/nid.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>

namespace prospero::modules {

using HleFunction = std::function<u64(u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)>;

struct HleExport {
    nid_t nid;
    std::string name;
    HleFunction func;
    u64 call_count{0};
};

class HleModule {
public:
    explicit HleModule(std::string name) : m_name(std::move(name)) {}
    virtual ~HleModule() = default;

    virtual bool init() = 0;
    const std::string& getName() const { return m_name; }

    void registerExport(nid_t nid, const std::string& name, HleFunction func) {
        m_exports[nid] = {nid, name, std::move(func), 0};
        NidDatabase::instance().registerNid(nid, name, m_name);
    }

    const HleExport* findExport(nid_t nid) const {
        auto it = m_exports.find(nid);
        if (it != m_exports.end()) return &it->second;
        return nullptr;
    }

    HleExport* findExport(nid_t nid) {
        auto it = m_exports.find(nid);
        if (it != m_exports.end()) return &it->second;
        return nullptr;
    }

    const std::unordered_map<nid_t, HleExport>& getExports() const { return m_exports; }

protected:
    std::string m_name;
    std::unordered_map<nid_t, HleExport> m_exports;
};

class ModuleManager {
public:
    static ModuleManager& instance() {
        static ModuleManager s_instance;
        return s_instance;
    }

    bool init();
    void registerModule(std::shared_ptr<HleModule> module);
    std::shared_ptr<HleModule> getModule(const std::string& name);

    template <typename T>
    std::shared_ptr<T> getModuleAs(const std::string& name) {
        return std::dynamic_pointer_cast<T>(getModule(name));
    }

    // Resolve NID across all registered modules
    u64 callNid(nid_t nid, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);
    bool hasNid(nid_t nid) const;
    std::string resolveName(nid_t nid) const;

    void dumpModules() const;

private:
    ModuleManager() = default;
    std::unordered_map<std::string, std::shared_ptr<HleModule>> m_modules;
    std::unordered_map<nid_t, std::shared_ptr<HleModule>> m_nid_to_module;
};

} // namespace prospero::modules
