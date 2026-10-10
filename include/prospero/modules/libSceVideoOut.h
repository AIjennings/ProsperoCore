#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

class LibSceVideoOut : public HleModule {
public:
    LibSceVideoOut();
    bool init() override;

    u64 getFlipCount() const { return m_flip_count; }

private:
    u64 m_flip_count{0};
};

} // namespace prospero::modules
