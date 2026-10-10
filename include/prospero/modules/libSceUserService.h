#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

class LibSceUserService : public HleModule {
public:
    LibSceUserService();
    bool init() override;
};

} // namespace prospero::modules
