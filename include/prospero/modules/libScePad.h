#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

#pragma pack(push, 1)
struct ScePadData {
    u32 buttons;
    u8  left_stick_x;
    u8  left_stick_y;
    u8  right_stick_x;
    u8  right_stick_y;
    u8  analog_l2;
    u8  analog_r2;
    u16 padding;
    u32 connected;
    u64 timestamp;
};
#pragma pack(pop)

class LibScePad : public HleModule {
public:
    LibScePad();
    bool init() override;
};

} // namespace prospero::modules
