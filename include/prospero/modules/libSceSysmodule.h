#pragma once

#include "prospero/modules/module_manager.h"
#include <unordered_set>

namespace prospero::modules {

class LibSceSysmodule : public HleModule {
public:
    LibSceSysmodule();
    bool init() override;

private:
    std::unordered_set<u32> m_loaded_modules;
};

} // namespace prospero::modules
