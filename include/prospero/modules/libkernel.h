#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

class LibKernel : public HleModule {
public:
    LibKernel();
    bool init() override;

private:
    void registerMemoryExports();
    void registerThreadExports();
    void registerEqueueExports();
    void registerTimeExports();
};

} // namespace prospero::modules
