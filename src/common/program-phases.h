#pragma once

#include <aem/types.h>
#include <vector>

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
        FC(std::size_t id, float current, float power)
            : id(id), current(current), power(power)
        { }

        VmOpType cmd() const override { return VmOpType::FC; }

        std::size_t id;
        float current;
        float power;
    };

    struct Pos
        : public Item 
    {
        Pos(char axis, float pos)
            : axis(axis), pos(pos)
        { }

        VmOpType cmd() const override { return VmOpType::Pos; }

        char axis;
        float pos;
    };

    //! Храним в рад/мин
    struct Spin
        : public Item 
    {
        Spin(char axis, float speed)
            : axis(axis), speed(speed)
        { }

        VmOpType cmd() const override { return VmOpType::Spin; }

        char axis;
        float speed;
    };

    //! Храним в мм/мин
    struct Speed
        : public Item 
    {
        Speed(float speed)
            : speed(speed)
        { }

        VmOpType cmd() const override { return VmOpType::Speed; }

        float speed;
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
    VmPause(aem::uint64 msec)
        : msec(msec)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::Pause; }

    // Время паузы в милисекундах
    aem::uint64 msec;
};

struct VmTimedFC
    : public VmPhase
{
    VmTimedFC(float _p, float _i, float _tv)
        : p(_p), i(_i), tv(_tv)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::TimedFC; }

    // Мощность
    float p;
    // Ток
    float i;
    // Время работы
    float tv;
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
    VmCenterTooth(int dir, float shift)
        : dir(dir), shift(shift)
    { }

    VmPhaseType cmd() const override { return VmPhaseType::CenterTooth; }

    // dir - internal(+1) or external(-1) tooth 
    int dir;

    // Отступ от выемки зуба
    float shift;
};


