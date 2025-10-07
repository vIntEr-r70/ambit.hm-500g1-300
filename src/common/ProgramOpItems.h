#pragma once

#include <string>
#include <sstream>
#include <limits>
#include <cstdint>

//#include <Ex/Buffer.h>

#include "Program.h"
#include "UnitsCalc.h"

#include "VmOp.h"

//--------------------------------------------------------------------------------------------------------------------

struct Phase
{
    virtual ~Phase() = default;

    virtual std::string asString() const { return "!"; }

    virtual uint32_t asColor() const { return 0x000000; }

    // virtual void makeByteCode(Ex::Buffer& buf) const {
    //     buf << cmd();
    // }

    virtual void writeArgs(std::stringstream&) const = 0;

    virtual std::string name() const = 0;

    virtual uint8_t cmd() const = 0;
};

struct Op
    : public Phase
{
    Op(bool _absolute)
        : Phase()
        , absolute(_absolute)
    { }

    void writeArgs(std::stringstream& fs) const override {
        fs << " absolute=\"" << (absolute ? "true" : "false") << "\"";
    }

    std::string name() const override { return "Op"; }

    uint8_t cmd() const override { return VmOp::OpOp; }

    bool absolute;
};

struct Pause
    : public Phase
{
    Pause(uint32_t _s, uint32_t _ds = 0)
        : s(_s), ds(_ds)
    { }

    std::string asString() const override
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Пауза [ %02u:%02u:%02u.%01u ]",
                s / 3600, (s % 3600) / 60, s % 60, ds);
        return buf;
    }

    void writeArgs(std::stringstream& fs) const override {
        fs << " seconds=\"" << s << "\" ds=\"" << ds << "\"";
    }

    // void makeByteCode(Ex::Buffer& buf) const override
    // {
    //     // В первом байте лежат доли секунды
    //     uint32_t arg(s | (ds << 24));
    //     buf << cmd() << arg;
    // }

    std::string name() const override { return "Pause"; }

    uint8_t cmd() const override { return VmOp::OpPause; }

    uint32_t asColor() const override { return (s || ds) ? 0x98FB98 : 0xFA8072; }

    uint32_t s;

    // Десятая часть секунды
    uint32_t ds;
};


struct TimedFC
    : public Phase
{
    TimedFC(float _p, float _i, float _tv)
        : p(_p), i(_i), tv(_tv)
    { }

    std::string asString() const override
    {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "Прецизионный нагрев [ Мощность: %.2f кВт, Ток: %.2f А, Время: %.1f секунд ]", p, i, tv);
        return buf;
    }

    void writeArgs(std::stringstream& fs) const override
    {
        fs << " p=\""  << p  << "\" " <<
              " i=\""  << i  << "\" " <<
              " tv=\"" << tv << "\"";
    }

    // void makeByteCode(Ex::Buffer& buf) const override {
    //     buf << cmd() << p << i << tv;
    // }

    std::string name() const override { return "TimedFc"; }

    uint8_t cmd() const override { return VmOp::OpTimedFC; }

    uint32_t asColor() const override { return 0xffd700; }

    float p;
    float i;
    float tv;
};


//--------------------------------------------------------------------------------------------------------------------

struct GoTo
    : public Phase
{
    GoTo(uint32_t _opid, uint32_t _N)
        : opid(_opid), N(_N)
    { }

    std::string asString() const override
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Переход на [ %u ] N=%u", opid + 1, N);
        return buf;
    }

    void writeArgs(std::stringstream& fs) const override
    {
        fs << " opid=\"" << opid << "\"";
        fs << " N=\"" << N << "\"";
    }

    // void makeByteCode(Ex::Buffer& buf) const override {
    //     buf << cmd() << opid << N;
    // }

    std::string name() const override { return "GoTo"; }

    uint8_t cmd() const override { return VmOp::OpGoTo; }

    uint32_t asColor() const override { return 0x87CEEB; }

    uint32_t opid;
    uint32_t N;
};



struct OpItem
{
protected:

    OpItem(std::size_t _priority)
        : priority(_priority)
    { }

public:

    virtual ~OpItem() = default;

    virtual OpItem* copy() const = 0;

    // virtual void makeByteCode(Ex::Buffer& buf) const = 0;
    virtual uint8_t cmd() const = 0;

    virtual void writeArgs(std::stringstream&) const = 0;

    virtual std::string asString() const noexcept { return "- - -"; }

    virtual bool isDiffer(OpItem const*) const { return false; }

    virtual bool isDefault() const { return true; }

    std::size_t priority;
};

struct OpBool
    : public OpItem
{
protected:

    OpBool(bool _state, std::size_t _priority)
        : OpItem(_priority)
        , state(_state)
    { }

public:

    inline void invert() { state = !state; }

    // void makeByteCode(Ex::Buffer& buf) const override {
    //     buf << cmd() << state;
    // }

    void writeArgs(std::stringstream& fs) const override {
        fs << "state=\"" << state << "\"";
    }

    bool isDiffer(OpItem const* opItem) const override {
        return dynamic_cast<OpBool const*>(opItem)->state != state;
    }

    bool isDefault() const override { return state == false; }

    bool state = false;
};

struct OpFloat
    : public OpItem
{
protected:

    OpFloat(float _value, std::size_t _priority)
        : OpItem(_priority)
        , value(_value)
    { }

public:

    std::string asString() const noexcept override
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", get());
        return std::string(buf);
    }

    // void makeByteCode(Ex::Buffer& buf) const override {
    //     buf << cmd() << value;
    // }

    void writeArgs(std::stringstream& fs) const override {
        fs << "value=\"" << value << "\"";
    }

    bool isDiffer(OpItem const* opItem) const override {
        return dynamic_cast<OpFloat const*>(opItem)->value != value;
    }

    bool isDefault() const override { return value == 0.f; }

    virtual void set(float v) { value = v; }

    void rawSet(float v) { value = v; }

    float rawGet() const { return value; }

    virtual float get() const { return value; }

protected:

    float value;
};

struct OpAxis
    : public OpFloat
{
protected:

    OpAxis(char _id, float _value, std::size_t _priority)
        : OpFloat(_value, _priority)
        , id(_id)
    { }

public:

    // void makeByteCode(Ex::Buffer& buf) const override {
    //     buf << cmd() << id << OpFloat::value;
    // }

    void writeArgs(std::stringstream& fs) const override
    {
        fs << "id=\"" << id << "\" ";
        OpFloat::writeArgs(fs);
    }

    char id;
};

struct OpSprayer
    : public OpBool
{
    OpSprayer(bool _state)
        : OpBool(_state, 0)
    { }

    OpItem* copy() const override { return new OpSprayer(OpBool::state); }

    std::string asString() const noexcept override
    {
        return OpBool::state ? "ВКЛ" : "ВЫКЛ";
    }

    uint8_t cmd() const override { return VmOp::OpSprayer; }
};

//! Храним в рад/мин
struct OpSpin
    : public OpAxis
{
    OpSpin(char _id, float _value)
        : OpAxis(_id, _value, 1)
    { }

    //! Отдаем в об/мин
    float get() const override { return UnitsCalc::toSpeed(true, value); }

    //! Получаем в об/мин
    void set(float v) override { value = UnitsCalc::fromSpeed(true, v); }

    OpItem* copy() const override { return new OpSpin(id, OpFloat::value); }

    uint8_t cmd() const override { return VmOp::OpSpin; }
};

struct OpCurrent
    : public OpFloat
{
    OpCurrent(float _value)
        : OpFloat(_value, 2)
    { }

    OpItem* copy() const override { return new OpCurrent(OpFloat::value); }

    uint8_t cmd() const override { return VmOp::OpCurrent; }
};

struct OpPower
    : public OpFloat
{
    OpPower(float _value)
        : OpFloat(_value, 3)
    { }

    OpItem* copy() const override { return new OpPower(OpFloat::value); }

    uint8_t cmd() const override { return VmOp::OpPower; }
};

struct OpPos
    : public OpAxis
{
    OpPos(char _id, bool _muGrads, float _value)
        : OpAxis(_id, _value, 4)
        , muGrads(_muGrads)
    { }

    OpItem* copy() const override { return new OpPos(id, muGrads, OpFloat::value); }

    uint8_t cmd() const override { return VmOp::OpPos; }

    // void makeByteCode(Ex::Buffer& buf) const override
    // {
    //     OpAxis::makeByteCode(buf);
    //     buf << absolute;
    // }

    void writeArgs(std::stringstream& fs) const override
    {
        fs << "muGrads=\"" << (muGrads ? "true" : "false") << "\" ";
        OpAxis::writeArgs(fs);
    }

    //! Отдаем в мм или град
    float get() const override { return UnitsCalc::toPos(muGrads, value); }

    //! Получаем в мм или град
    void set(float v) override { value = UnitsCalc::fromPos(muGrads, v); }

    bool isDiffer(OpItem const* prev) const override
    {
        return absolute ? OpAxis::isDiffer(prev) : !OpAxis::isDefault();
    }

    bool absolute{ true };
    bool muGrads{ false };
};

//! Храним в мм/мин
struct OpSpeed
    : public OpFloat
{
    OpSpeed(bool _muGrads, float _value)
        : OpFloat(_value, 5)
        , muGrads(_muGrads)
    { }

    void writeArgs(std::stringstream& fs) const override
    {
        fs << "muGrads=\"" << (muGrads ? "true" : "false") << "\" ";
        OpFloat::writeArgs(fs);
    }

    //! Отдаем в мм/с
    float get() const override { return UnitsCalc::toSpeed(muGrads, value); }

    //! Получаем в мм/с
    void set(float v) override { value = UnitsCalc::fromSpeed(muGrads, v); }

    OpItem* copy() const override { return new OpSpeed(muGrads, OpFloat::value); }

    uint8_t cmd() const override { return VmOp::OpSpeed; }

    bool muGrads{ false };
};

