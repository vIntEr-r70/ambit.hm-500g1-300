#pragma once

#include <vector>
#include <cstdint>

//--------------------------------------------------------------------------------------------------------------------

enum class VmPhaseType
{
    Operation,
    Pause,
    GoTo,
    TimedFC,
    CenterShaft,
    CenterTooth
};

enum class VmOpType
{
    Pos,
    Spin,
    Speed,
    FC,
    Sprayer,
};

//--------------------------------------------------------------------------------------------------------------------

struct VmPhase
{
    virtual ~VmPhase() = default;

    virtual VmPhaseType cmd() const = 0;
};

struct VmOperation
    : public VmPhase
{
    struct Item
    {
    protected:

        Item()
        { }

    public:

        virtual ~Item() = default;
        virtual VmOpType cmd() const = 0;
    };

    struct Sprayer
        : public Item 
    {
        Sprayer(std::size_t id, bool state)
            : id(id), state(state)
        { }

        VmOpType cmd() const override { return VmOpType::Sprayer; }

        std::size_t id;
        bool state;
    };

    struct FC
        : public Item 
    {
        FC(std::size_t id, double current, double power)
            : id(id), current(current), power(power)
        { }

        VmOpType cmd() const override { return VmOpType::FC; }

        std::size_t id;
        double current;
        double power;
    };

    struct Pos
        : public Item 
    {
        Pos(char axis, double pos)
            : axis(axis), pos(pos)
        { }

        VmOpType cmd() const override { return VmOpType::Pos; }

        char axis;
        double pos;
    };

    //! Храним в рад/мин
    struct Spin
        : public Item 
    {
        Spin(char axis, double speed)
            : axis(axis), speed(speed)
        { }

        VmOpType cmd() const override { return VmOpType::Spin; }

        char axis;
        double speed;
    };

    //! Храним в мм/мин
    struct Speed
        : public Item 
    {
        Speed(double speed)
            : speed(speed)
        { }

        VmOpType cmd() const override { return VmOpType::Speed; }

        double speed;
    };

    VmOperation(bool _absolute)
        : VmPhase()
        , absolute(_absolute)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::Operation; }

    // Список операций
    std::vector<Item*> items;

    // Позиция в операции задана относительно или абсолютно
    bool absolute;
};

struct VmPause
    : public VmPhase
{
    VmPause(std::uint64_t msec)
        : msec(msec)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::Pause; }

    // Время паузы в милисекундах
    std::uint64_t msec;
};

struct VmTimedFC
    : public VmPhase
{
    VmTimedFC(double _p, double _i, double _tv)
        : p(_p), i(_i), tv(_tv)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::TimedFC; }

    // Мощность
    double p;
    // Ток
    double i;
    // Время работы
    double tv;
};

//--------------------------------------------------------------------------------------------------------------------

struct VmGoTo
    : public VmPhase
{
    VmGoTo(unsigned int _phase_id, unsigned int _N)
        : phase_id(_phase_id), N(_N)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::GoTo; }

    // На какой этап перейти c данного
    unsigned int phase_id;
    // Сколько раз
    unsigned int N;
};

//--------------------------------------------------------------------------------------------------------------------

struct VmCenterShaft
    : public VmPhase
{
    VmCenterShaft()
    { }

    VmPhaseType cmd() const override { return VmPhaseType::CenterShaft; }
};

//--------------------------------------------------------------------------------------------------------------------

struct VmCenterTooth
    : public VmPhase
{
    VmCenterTooth(int dir, double shift)
        : dir(dir), shift(shift)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::CenterTooth; }

    // dir - internal(+1) or external(-1) tooth 
    int dir;

    // Отступ от выемки зуба
    double shift;
};


