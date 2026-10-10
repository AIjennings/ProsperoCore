#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

class LibSceAudioOut : public HleModule {
public:
    LibSceAudioOut();
    bool init() override;
};

} // namespace prospero::modules
