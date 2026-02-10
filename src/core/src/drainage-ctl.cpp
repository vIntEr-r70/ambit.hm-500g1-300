#include "drainage-ctl.h"

// Данная система контроллирует уровень жидкости в некой емкости
// основываясь на значениях датчиков уровня.
// Поведение:
// Датчик max: включаем насос
// Датчик min: выключаем насос
// Так-же в системе присутствует клапан, который может
// находится в состояниях [подача в канализацию, переключение, подача в емкость]

drainage_ctl::drainage_ctl()
    : eng::sibus::node("drainage-ctl")
    // , H7_ctl_(hw.get_unit_by_class(sid, "H7"), "H7")
{
    // hw.LSET.add(sid, "min", [this](nlohmann::json const &value) 
    // {
    //     if (value.get<bool>())
    //     {
    //         hw_.log_message(LogMsg::Warning, name(), "Уровень достиг минимума, выключаем насос");
    //         pump_ = false;
    //     }
    // });

    // hw.LSET.add(sid, "max", [this](nlohmann::json const &value) 
    // {
    //     if (value.get<bool>())
    //     {
    //         hw_.log_message(LogMsg::Warning, name(), "Уровень достиг максимума, включаем насос");
    //         pump_ = true;
    //     }
    // });

    // hw.LSET.add(sid, "H7", [this](nlohmann::json const &value) 
    // {
    //     hw_.log_message(LogMsg::Info, name(), 
    //         fmt::format("Насос H7 {}", value.get<bool>() ? "включен" : "выключен"));
    // });
}

// void drainage_ctl::update_state()
// {
//     if (насос включен)
// }

// void drainage_ctl::on_activate() noexcept
// {
//     hw_.log_message(LogMsg::Info, name(), "Контроллер успешно активирован");
//
//     pump_on_off_ = false;
//     next_state(&drainage_ctl::turn_on_off_pump);
// }

// void drainage_ctl::on_deactivate() noexcept
// { 
//     hw_.log_message(LogMsg::Info, name(), "Контроллер деактивирован");
// }

// void drainage_ctl::wait_state_changed()
// {
//     if (!pump_) return;
//
//     pump_on_off_ = pump_.value();
//     pump_.reset();
//
//     next_state(&drainage_ctl::turn_on_off_pump);
// }

// void drainage_ctl::turn_on_off_pump()
// {
//     auto [ done, error ] = H7_ctl_.set(pump_on_off_);
//     if (!dhresult::check(done, error, [this] { next_state(&drainage_ctl::turn_on_off_pump_error); }))
//         return;
//     // hw_.log_message(LogMsg::Info, name(), fmt::format("Насос {}", pump_on_off_ ? "включен" : "выключен"));
//     next_state(&drainage_ctl::wait_state_changed);
// }

// void drainage_ctl::turn_on_off_pump_error()
// {
//     hw_.log_message(LogMsg::Error, name(), "Не удалось выполнить команду");
//     next_state(&drainage_ctl::wait_state_changed);
// }


