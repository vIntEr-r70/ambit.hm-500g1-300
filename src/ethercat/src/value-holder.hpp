#pragma once

#include <functional>
#include <cstddef>
#include <cstdint>
#include <cstring>

class value_holder_base
{
protected:

    // Обработчик изменения
    std::function<void()> on_changed_;

public:

    void on_changed(std::function<void()> on_changed)
    {
        on_changed_ = std::move(on_changed);
    }

public:

    virtual std::size_t size() const noexcept = 0;

    virtual void update(std::uint8_t const *) = 0;

    virtual bool upload_value(std::uint8_t *) = 0;
};

class value_holder_base_rw
    : public value_holder_base
{ };

class value_holder_base_ro
    : public value_holder_base
{ };

template <typename T>
class value_holder_rw
    : public value_holder_base_rw
{
    // Память для хранения значения
    T value_{};

    T upload_value_{};
    bool upload_set_{ false };

public:

    value_holder_rw() = default;

public:

    T get() const noexcept
    {
        return value_;
    }

    void set(T value) noexcept
    {
        upload_value_ = value;
        upload_set_ = true;
    }

private:

    std::size_t size() const noexcept override {
        return sizeof(T);
    }

    void update(std::uint8_t const *data) override
    {
        std::memcpy(&value_, data, sizeof(T));
        if (on_changed_) on_changed_();
    }

    bool upload_value(std::uint8_t *data) override
    {
        if (!upload_set_)
            return false;

        upload_set_ = false;
        std::memcpy(data, &upload_value_, sizeof(T));

        return true;
    }
};

template <typename T>
class value_holder_ro
    : public value_holder_base_ro
{
    // Память для хранения значения
    T value_;

public:

    value_holder_ro() = default;

public:

    T get() const noexcept { return value_; }

private:

    std::size_t size() const noexcept override {
        return sizeof(T);
    }

    void update(std::uint8_t const *data) override
    {
        std::memcpy(&value_, data, sizeof(T));
        if (on_changed_) on_changed_();
    }

    bool upload_value(std::uint8_t *) override
    { return false; }
};
