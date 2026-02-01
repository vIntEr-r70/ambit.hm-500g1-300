#pragma once

#include <eng/timer.hpp>

template <typename T>
class internal_state_ctl
{
    typedef void(T::*state_handler_t)();

    state_handler_t state_{ nullptr };
    eng::timer::id_t state_call_timer_;

protected:

    void switch_to_state(state_handler_t new_state)
    {
        state_ = new_state;

        if (state_call_timer_)
        {
            eng::timer::kill_timer(state_call_timer_);
            state_call_timer_.reset();
        }

        if (state_ == nullptr) return;

        state_call_timer_ = eng::timer::once([this]
        {
            state_call_timer_.reset();
            touch_current_state();
        });
    }

    bool touch_current_state()
    {
        if (state_ == nullptr)
            return false;

        // Если вызов произошел раньше чем это сделал таймер
        if (state_call_timer_)
        {
            // Отменяем таймер
            eng::timer::kill_timer(state_call_timer_);
            state_call_timer_.reset();
        }

        // Производим вызов
        (static_cast<T*>(this)->*state_)();

        return true;
    };

    bool is_in_state() const noexcept
    {
        return state_ != nullptr;
    }
};
