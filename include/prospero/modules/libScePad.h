#pragma once

#include "prospero/modules/module_manager.h"

namespace prospero::modules {

enum ScePadButtons : u32 {
    SCE_PAD_L3        = 0x00000002,
    SCE_PAD_R3        = 0x00000004,
    SCE_PAD_OPTIONS   = 0x00000008,
    SCE_PAD_UP        = 0x00000010,
    SCE_PAD_RIGHT     = 0x00000020,
    SCE_PAD_DOWN      = 0x00000040,
    SCE_PAD_LEFT      = 0x00000080,
    SCE_PAD_L2        = 0x00000100,
    SCE_PAD_R2        = 0x00000200,
    SCE_PAD_L1        = 0x00000400,
    SCE_PAD_R1        = 0x00000800,
    SCE_PAD_TRIANGLE  = 0x00001000,
    SCE_PAD_CIRCLE    = 0x00002000,
    SCE_PAD_CROSS     = 0x00004000,
    SCE_PAD_SQUARE    = 0x00008000,
    SCE_PAD_TOUCH_PAD = 0x00100000,
};

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

    static void pollHostInput(ScePadData& pad_out);
    static void setHapticVibration(u8 large_motor, u8 small_motor);
    static void getHapticState(u8& large_motor, u8& small_motor);

private:
    static u8 s_large_motor;
    static u8 s_small_motor;
};

} // namespace prospero::modules
