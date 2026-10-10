#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

class LibSceSystemService : public HleModule {
public:
    LibSceSystemService();
    bool init() override;
};

} // namespace prospero::modules
