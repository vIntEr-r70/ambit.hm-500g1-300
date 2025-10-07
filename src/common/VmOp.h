#pragma once

struct VmOp
{
    enum : uint8_t
    {
        OpPos = 0,
        OpSpin,
        OpSpeed,
        OpPower,
        OpCurrent,
        OpSprayer,

        OpOp,
        OpPause,
        OpGoTo,
        OpTimedFC,

        OpsCount
    };
};
