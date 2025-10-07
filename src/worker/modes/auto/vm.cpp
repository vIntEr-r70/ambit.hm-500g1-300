#include "vm.h"
#include "flags.h"

#include <cmath>
#include <hardware.h>

#include <aem/log.h>

vm::vm(we::hardware &hw, we::axis_cfg const &axis_cfg) noexcept
    : state_ctrl("vm")
    , hw_(hw)
    , cnc_(hw.get_unit_by_class("", "cnc"))
    , fc_(hw.get_unit_by_class("", "fc"))
    // , pump_(hw.get_unit_by_class("spump", "power"), "spump")
    , sprayer_{{ 
        { hw_.get_unit_by_class("Sp-0", "power"), "Sp-0" }, 
        { hw_.get_unit_by_class("Sp-1", "power"), "Sp-1" }, 
        { hw_.get_unit_by_class("Sp-2", "power"), "Sp-2" } }}
    , a_centering_(hw, axis_cfg)
{
    map_phases_[VmPhaseType::Operation]     = &vm::_vm_operation;
    map_phases_[VmPhaseType::Pause]         = &vm::_vm_pause;
    map_phases_[VmPhaseType::GoTo]          = &vm::_vm_goto;
    map_phases_[VmPhaseType::TimedFC]       = &vm::_vm_timedfc;
    map_phases_[VmPhaseType::CenterShaft]   = &vm::_vm_center_shaft;
    map_phases_[VmPhaseType::CenterTooth]   = &vm::_vm_center_tooth;

    map_operations_[VmOpType::Pos]      = &vm::_vm_op_pos;
    map_operations_[VmOpType::Spin]     = &vm::_vm_op_spin;
    map_operations_[VmOpType::Speed]    = &vm::_vm_op_speed;
    map_operations_[VmOpType::FC]       = &vm::_vm_op_fc;
    map_operations_[VmOpType::Sprayer]  = &vm::_vm_op_sprayer;

    map_pos_operations_[VmOpType::Pos]      = &vm::_vm_next_op_pos;
    map_pos_operations_[VmOpType::Speed]    = &vm::_vm_next_op_speed;

    // Загружаем значение уставки напряжения из хранилища
    // fc_.set_param(engine::fc_reg_driver::U, hw.storage("fc", "U-SET", 400));

    // Загружаем граничные значение уставок из хранилища
    // fc_.cfg_limit(engine::fc_reg_driver::Pmax, hw.storage("fc", "P-MAX", 50000.0f));
    // fc_.cfg_limit(engine::fc_reg_driver::Pmin, hw.storage("fc", "P-MIN", 500.0f));
    // fc_.cfg_limit(engine::fc_reg_driver::Imax, hw.storage("fc", "I-MAX", 150.0f));
}

bool vm::set_program(std::vector<VmPhase*> &phases, 
        std::vector<char> &spin_axis, 
        std::vector<char> &target_axis,
        bool use_fc, bool use_sprayers) noexcept
{
    std::size_t nz_count = 0;
    
    // Убеждаемся что все необходимые оси находятся в 0 координат
    std::for_each(target_axis.begin(), target_axis.end(), [this, &nz_count](char axis) 
    {
        float pos = cnc_.pos(axis);
        if (std::fpclassify(pos) == FP_ZERO)
            return;
        hw_.log_message(LogMsg::Error, "auto", fmt::format("Ось {} не в 0!", axis));
        nz_count += 1;
    });

    if (nz_count)
        return false;

    phases_.swap(phases);

    spin_axis_.swap(spin_axis);
    target_axis_.swap(target_axis);

    // Инициализируем начальную позицию
    from_position_.clear();
    std::for_each(target_axis_.begin(), target_axis_.end(), [this](char axis) {
        from_position_[axis] = 0.0f;
    });

    use_fc_ = use_fc;
    use_sprayers_ = use_sprayers;

    return true;
}

bool vm::is_in_infinity_pause() const noexcept
{
    return is_current_state(&vm::infinite_pause);
}

void vm::do_continue() noexcept
{
    hw_.nf_.notify({ "auto-mode", "pause" }, false);
    next_state(&vm::phase_done);
}

void vm::on_activate() noexcept
{
    // Все параметры в состояния по умолчанию
    // --------------------------------------
    phase_id_ = 0;
    hw_.nf_.notify({ "auto-mode", "phase-id" }, phase_id_);

    target_position_.clear();
    next_target_position_.clear();
    last_position_.clear();

    spin_.clear();

    // fc_.set_param(engine::fc_reg_driver::P, 0.0f);
    // fc_.set_param(engine::fc_reg_driver::I, 0.0f);
    fc_powered_ = false;

    next_state(use_sprayers_ ? &vm::turn_on_sprayers_pump : &vm::set_axis_spin_mode);
}

void vm::on_deactivate() noexcept
{
    next_state(&vm::stop_program);
}

void vm::on_deactivated() noexcept
{
    // Снимаем Блокировку срабатывания глобального контроля касания 
    hw_.set_flag(flags::bki_lock_ignore, false);
    aem::log::error("---------------vm::on_deactivated: reset bki_lock_ignore");
    hw_.nf_.notify({ "auto-mode", "phase-id" }, {});
}

void vm::turn_on_sprayers_pump() noexcept
{
    aem::log::warn("---------------vm::turn_on_sprayers_pump");
    hw_.send_event("auto", "start", true);
    next_state(&vm::set_axis_spin_mode);
}

void vm::set_axis_spin_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_independent_mode(spin_axis_);
    if (!dhresult::check(done, error, [this] { set_op_error("Маска осей независимого движения"); }))
        return;
    next_state(&vm::set_axis_target_mode);
}

void vm::set_axis_target_mode() noexcept
{
    auto [ done, error ] = cnc_.switch_to_target_mode(target_axis_);
    if (!dhresult::check(done, error, [this] { set_op_error("Маска осей позиционного движения"); }))
        return;

    aem::log::warn("--> vm::phase_done");
    next_state(&vm::phase_done);
}

// В процессе движения к цели проверяем возможность и необходимость добавить следующую позицию
// Чтобы двигатели лишний раз не останавливались
void vm::try_add_next_target() noexcept
{
    aem::log::warn("vm::try_add_next_target");

    // Драйвер передал команды на устройство
    // Теперь мы можем добавить новые координаты
    // Проверяем можем ли мы это сделать

    // Находим следующию операцию с учетом циклов
    VmPhase const* phase = get_next_operation(phase_id_ + 1);

    // Если сейчас выполняется последний этап либо следующци этап не операция
    if (phase == nullptr)
    {
        aem::log::warn("\tvm::try_add_next_target FAIL");

        // Просто идем на ожидание завершения движения, остановки не миновать
        next_state(&vm::moving_to_target);
        return;
    }

    // Формируем следующую позицию для cnc согласно операции
    VmOperation const &item = *static_cast<VmOperation const*>(phase);

    // Применяем только операции позиционирования
    for (VmOperation::Item *op : item.items)
    {
        auto it = map_pos_operations_.find(op->cmd());
        if (it == map_pos_operations_.end())
            continue;
        std::invoke(it->second, this, op, item.absolute);
    }

    // Проверяем что следующая позиция не содержит инверсии движения 
    auto it = std::find_if(next_target_position_.begin(), next_target_position_.end(), [this](auto const& pair)
    {
        float f = from_position_[pair.first];
        float p = target_position_[pair.first];
        float n = pair.second; 
        return ((n <= f) && ((p < n) || (p > f))) || ((f <= n) && ((p < f) || (p > n)));
    });

    // Если нашли, не передаем команду драйверу
    if (it != next_target_position_.end())
    {
        aem::log::warn("\tvm::try_add_next_target FAIL by inversion");

        next_target_position_.clear();
        next_state(&vm::moving_to_target);
        return;
    }

    aem::log::warn("\tvm::try_add_next_target OK");

    // Отдаем следующую позицию CNC
    next_state(&vm::_cnc_next_target_move);
}

// Проверяем что мы достигли необходимой позиции
void vm::moving_to_target() noexcept
{
#ifdef EXT_LOG
    std::vector<std::string> msgs;
#endif
    
    std::size_t in_target = 0;
    for (auto const &item : target_position_)
    {
        char axis = item.first;
        float from = from_position_[axis];
        float next = last_position_[axis];
        float target = item.second;

        // Сравниваем текущее положение с целевым
        if (cnc_.compare_pos(axis, from, target, next))
        {
            in_target += 1;
#ifdef EXT_LOG
            msgs.push_back(fmt::format("DONE[{}]: from: {} -> to: {} -> next: {} -> pos: {}", axis, from, target, next, cnc_.pos(axis)));
#endif
        }
#ifdef EXT_LOG
        else
        {
            msgs.push_back(fmt::format("PROC[{}]: from: {} -> to: {} -> next: {} -> pos: {}", axis, from, target, next, cnc_.pos(axis)));
        }
#endif
    }

    // Все целевые оси достигли целевого положения, идем на следующий шаг
    if (in_target == target_position_.size())
    {
        for (auto const &item : target_position_)
        {
#ifdef EXT_LOG
            hw_.log_message(LogMsg::Warning, "auto", fmt::format("vm::in_target: {} = {}", item.first, item.second));
#endif
            aem::log::warn("vm::in_target: {} = {}", item.first, item.second);
        }

        // Смещаем текущую точку на позицию предидущей
        from_position_ = target_position_;

        phase_id_ += 1;

        aem::log::warn("--> vm::phase_done");
        next_state(&vm::phase_done);

        return;
    }

    // Если мы еще не достигли цели, убеждаемся что мы находимся в движении
    if (cnc_.status() != engine::cnc_status::idle)
       return;

#ifdef EXT_LOG
    for (auto const& msg : msgs)
        hw_.log_message(LogMsg::Warning, "auto", msg);
#endif
        
    // Если мы никуда не едем то это ошибка программы
    aem::log::error("Мы никуда не едем!");
    hw_.log_message(LogMsg::Error, "auto", "Мы никуда не едем!");

    next_state(&vm::stop_program);
}

// Контролируем прошедшее с момента начала выполнения паузы время
void vm::finite_pause() noexcept
{
    // Находимся на паузе, проверяем, не пора ли ее закончить
    if (!countdown_.elapsed())
    {
        hw_.nf_.notify({ "auto-mode", "time-pause" }, countdown_.left().seconds());
        return;
    }

    hw_.nf_.notify({ "auto-mode", "time-pause" }, 0);

    // Этап закончен, идем на следующий

    aem::log::warn("--> vm::phase_done");
    next_state(&vm::phase_done);
}

// Ожидаем команды на продолжение выполнения
void vm::infinite_pause() noexcept
{
    hw_.nf_.notify({ "auto-mode", "time-pause" }, 0);
}

void vm::timed_fc_0() noexcept
{
    // auto [ done, error ] = fc_.set();
    // if (!dhresult::check(done, error, [this] { set_op_error("Уставки ПЧ"); }))
    //     return;
    //
    // next_state(&vm::timed_fc_1);
}

void vm::timed_fc_1() noexcept
{
    //! Если мы включаемся, проверяем необходимость инициализации таймера
    // if (turnOn && fc->allowTimer())
    // {
    //     fc->makeRegsWrite(FC::RegTimer, ms ? 0x0001 : 0x0000);
    //     if (ms != 0)
    //     {
    //         uint16_t const tv[2] { static_cast<uint16_t>(ms / 60000), static_cast<uint16_t>((ms % 60000) / 100) };
    //         fc->makeRegsWrite(FC::RegTimerSet, tv, 2);
    //
    //         Ex::Log::trace("Hw: timered Fc power on with: minutes={} ms*0.01{}", tv[0], tv[1]);
    //     }
    // }
    // fc->makeRegsWrite(FC::RegCtrl, turnOn ? 0x0001 : 0x0000);

    // auto [ done, error ] = hw_.fc_timed.set(tvms_);
    // if (!dhresult::check(done, error, [this] { set_op_error("ПЧ"); }))
    //     return;
    fc_error_ = false;
    fc_started_ = false;

    next_state(&vm::timed_fc_2);
}

void vm::timed_fc_2() noexcept
{
    if (!fc_error_ && fc_started_)
        return;

    phase_id_ += 1;

    // Этап закончен, идем на следующий

    aem::log::warn("--> vm::phase_done");
    next_state(&vm::phase_done);
}

void vm::centering() noexcept
{
    aem::log::warn("vm::centering");

    if (!a_centering_.touch() && !a_centering_.is_error())
        return;

    // Снимаем Блокировку срабатывания глобального контроля касания 
    hw_.set_flag(flags::bki_lock_ignore, false);
    aem::log::error("---------------vm::centering: reset bki_lock_ignore");

    if (a_centering_.is_error())
    {
        set_op_error("Центровка");
        return;
    }

    phase_id_ += 1;

    // Этап закончен, идем на следующий
    aem::log::warn("--> vm::phase_done");
    next_state(&vm::phase_done);
}

// Проверяем, не пора ли закончить выполнение
// phase_id_ содержит индекс следующего для выполнения этапа
void vm::phase_done() noexcept
{
    hw_.nf_.notify({ "auto-mode", "phase-id" }, phase_id_);
    aem::log::trace("vm::phase_done: next phase = {}", phase_id_);

    // Больше выполнять нечего, приводим все в исходное состояние
    if (phase_id_ >= phases_.size())
    {
        hw_.log_message(LogMsg::Info, "auto", "Программа завершила выполнение");

        next_state(&vm::stop_program);
        return;
    }

    hw_.log_message(LogMsg::Info, "auto", fmt::format("Начало выполнения этапа №{}", phase_id_ + 1));

    // Инициируем выполнение следующего этапа выполняемой программы
    auto const* phase = phases_[phase_id_];
    std::invoke(map_phases_[phase->cmd()], this, phase);
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

// Находим следующию операцию с учетом циклов
VmPhase const* vm::get_next_operation(std::size_t pid) const noexcept
{
    if (pid == phases_.size())
    {
        aem::log::trace("vm::get_next_operation[{}]: FINISH", pid);
        return nullptr;
    }

    VmPhase const* phase = phases_[pid];
    if (phase->cmd() == VmPhaseType::Operation)
        return phase;

    if (phase->cmd() != VmPhaseType::GoTo)
    {
        aem::log::trace("vm::get_next_operation[{}]: STOP DETECT", pid);
        return nullptr;
    }

    // Исследуем этап, на который ведет переход при условии что количество повторений еще осталось
    VmGoTo const &item = *static_cast<VmGoTo const*>(phase);

    auto it = known_goto_.find(pid);
    std::size_t N = (it != known_goto_.end()) ? (it->second - 1) : item.N;

    // Операция перехода не актуальна, проверяем следующий за ней этап
    if (N == 0)
        return get_next_operation(pid + 1);

    // Проверяем этап, на который указывает операция перехода
    return get_next_operation(item.phase_id);
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

// Вызывается только после завершения предидущей операции
void vm::_vm_operation(VmPhase const* phase) noexcept
{
    VmOperation const &item = *static_cast<VmOperation const*>(phase);
    aem::log::debug("VmOperation:");

    // Применяем операции формируя внутренее состояние
    for (VmOperation::Item *op : item.items)
        std::invoke(map_operations_[op->cmd()], this, op, item.absolute);

    // Если до этого мы уже отдали команду на движение
    if (!next_target_position_.empty())
    {
        // Просто обновляем ожидаемые координаты
        for (auto const& pos : next_target_position_)
            target_position_[pos.first] = pos.second;
    }

    // Активируем состояние
    next_state(&vm::apply_sys_state);
}

void vm::_vm_pause(VmPhase const* phase) noexcept
{
    VmPause const &item = *static_cast<VmPause const*>(phase);
    aem::log::debug("VmPause: {} msec", item.msec);

    phase_id_ += 1;

    // Нам необходимо безсрочно остановить выполнение
    if (item.msec == 0)
    {
        hw_.nf_.notify({ "auto-mode", "pause" }, true);
        next_state(&vm::infinite_pause);
        return;
    }

    // Иначе запускаем таймер ожидания
    countdown_ = aem::countdown(aem::time_span::ms(item.msec));

    // Инициируем соответствующее состояние
    next_state(&vm::finite_pause);
}

void vm::_vm_goto(VmPhase const* phase) noexcept
{
    VmGoTo const &item = *static_cast<VmGoTo const*>(phase);
    aem::log::debug("VmGoTo: [{}, {}]", item.phase_id, item.N);

    // Пытаемся вставить как новое значение
    auto const result = known_goto_.insert({ phase_id_, item.N });
    auto it = result.first;

    // Если запись уже была, это очередной проход, уменьшаем счетчик
    if (result.second == false)
        it->second -= 1;

    // Если счетчик стал или был равен 0, мы закончили
    if (it->second == 0)
    {
        // Удаляем упоминание о себе
        known_goto_.erase(it);

        // Меняем текущий этап на следующий в списке
        phase_id_ += 1;
    }
    else
    {
        // Меняем текущий этап на нужную позицию
        phase_id_ = item.phase_id;
    }

    aem::log::warn("--> vm::phase_done");
    next_state(&vm::phase_done);
}

void vm::_vm_timedfc(VmPhase const* phase) noexcept
{
    VmTimedFC const &item = *static_cast<VmTimedFC const*>(phase);
    aem::log::debug("VmTimedFC: [p = {}, i = {}, tv = {}]", item.p, item.i, item.tv);

    // fc_.set_param(engine::fc_reg_driver::P, item.p);
    // fc_.set_param(engine::fc_reg_driver::I, item.i);

    aem::uint32 const ms = static_cast<aem::uint32>(item.tv * 1000);
    tvmc_ = { static_cast<aem::uint16>(ms / 60000), static_cast<aem::uint16>((ms % 60000) / 100) };

    next_state(&vm::timed_fc_0);
}

void vm::_vm_center_shaft(VmPhase const* phase) noexcept
{
    VmCenterShaft const &item = *static_cast<VmCenterShaft const*>(phase);
    aem::log::debug("VmCenterShaft: []");

    // Блокируем срабатывание глобального контроля касания 
    hw_.set_flag(flags::bki_lock_ignore, true);
    aem::log::error("---------------vm: SET bki_lock_ignore");

    a_centering_.init_shaft();
    next_state(&vm::centering);
}

void vm::_vm_center_tooth(VmPhase const* phase) noexcept
{
    VmCenterTooth const &item = *static_cast<VmCenterTooth const*>(phase);
    aem::log::debug("VmCenterTooth: [dir = {}, shift = {}]", item.dir, item.shift);

    // Блокируем срабатывание глобального контроля касания 
    hw_.set_flag(flags::bki_lock_ignore, true);
    aem::log::error("---------------vm: SET bki_lock_ignore");

    a_centering_.init_tooth(item.dir, item.shift);
    next_state(&vm::centering);
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void vm::_vm_op_pos(VmOperation::Item const* item, bool absolute) noexcept
{
    VmOperation::Pos const& op = *static_cast<VmOperation::Pos const*>(item);

    // Учитываем абсолютное или относительное значение
    if (absolute)
        target_position_[op.axis] = op.pos;
    else
        target_position_[op.axis] = from_position_[op.axis] + op.pos;
}

void vm::_vm_op_spin(VmOperation::Item const* item, bool) noexcept
{
    VmOperation::Spin const& op = *static_cast<VmOperation::Spin const*>(item);
    spin_[op.axis] = op.speed;
}

void vm::_vm_op_speed(VmOperation::Item const* item, bool) noexcept
{
    VmOperation::Speed const& op = *static_cast<VmOperation::Speed const*>(item);
    target_motion_speed_ = op.speed;
}

void vm::_vm_op_fc(VmOperation::Item const* item, bool) noexcept
{
    VmOperation::FC const& op = *static_cast<VmOperation::FC const*>(item);
    // fc_.set_param(engine::fc_reg_driver::P, op.power);
    // fc_.set_param(engine::fc_reg_driver::I, op.current);
    fc_powered_ = (std::fpclassify(op.power) != FP_ZERO);
}

void vm::_vm_op_sprayer(VmOperation::Item const* item, bool) noexcept
{
    VmOperation::Sprayer const& op = *static_cast<VmOperation::Sprayer const*>(item);
    sprayers_state_[op.id] = op.state;
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void vm::_vm_next_op_pos(VmOperation::Item const* item, bool absolute) noexcept
{
    VmOperation::Pos const& op = *static_cast<VmOperation::Pos const*>(item);
    next_target_position_[op.axis] = op.pos;

    // Если используется не абсолютное значение, прибавляем предидущее значение
    // Оно уже точно есть потому как хотябы одна операция выполнена
    if (!absolute)
        next_target_position_[op.axis] += target_position_[op.axis];
}

void vm::_vm_next_op_speed(VmOperation::Item const* item, bool) noexcept
{
    VmOperation::Speed const& op = *static_cast<VmOperation::Speed const*>(item);
    target_motion_speed_ = op.speed;
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void vm::set_op_error(std::string const& emsg) noexcept
{
    hw_.log_message(LogMsg::Error, "auto", emsg);
    next_state(&vm::op_error);
}

void vm::op_error() noexcept
{
    hw_.log_message(LogMsg::Error, "auto", "Программа завершила выполнение из-за ошибки");

    aem::log::error("vm::command_error");
    next_state(&vm::stop_program);
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void vm::_cnc_target_move() noexcept
{
    auto [ done, error ] = cnc_.target_move(target_position_, target_motion_speed_);
    if (!dhresult::check(done, error, [this] { set_op_error("Команда движения в позицию"); }))
        return;
    last_position_ = next_target_position_;

    next_state(&vm::try_add_next_target);
}

void vm::_cnc_next_target_move() noexcept
{
    auto [ done, error ] = cnc_.target_move(next_target_position_, target_motion_speed_);
    if (!dhresult::check(done, error, [this] { set_op_error("Упреждающая команда движения"); }))
        return;
    last_position_ = next_target_position_;

    // Идем на ожидание достижения нужной позиции
    next_state(&vm::moving_to_target);
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void vm::apply_sys_state() noexcept
{
    aem::log::trace("vm::apply_sys_state");

    sprayer_id_ = 0;

    if (spin_.empty())
        next_state(&vm::_update_sprayer_0);
    else
        next_state(&vm::_update_spin);
}

void vm::_update_spin() noexcept
{
    aem::log::trace("vm::_update_spin");
    auto [ done, error ] = cnc_.independent_move(spin_);
    if (!dhresult::check(done, error, [this] { set_op_error("Команда вращения"); }))
        return;
    next_state(&vm::_update_sprayer_0);
}

void vm::_update_sprayer_0() noexcept
{
    if (sprayer_id_ >= sprayers_state_.size() || !use_sprayers_)
    {
        next_state(&vm::_update_fc_0);
        return;
    }

    aem::log::trace("vm::update_sprayer_0: {}/{}", sprayer_id_, sprayers_state_.size());

    auto [ done, error ] = sprayer_[sprayer_id_].set(sprayers_state_[sprayer_id_]);
    if (!dhresult::check(done, error, [this] { set_op_error("Спрейер"); }))
        return;

    next_state(&vm::_update_sprayer_1);
}

void vm::_update_sprayer_1() noexcept
{
    aem::log::trace("vm::update_sprayer_1: {}", sprayer_id_);

    auto [ done, error ] = sprayer_[sprayer_id_].set(sprayers_state_[sprayer_id_]);
    if (!dhresult::check(done, error, [this] { set_op_error("Индикатор спрейера"); }))
        return;

    sprayer_id_ += 1;
    next_state(&vm::_update_sprayer_0);
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

// Записываем уставки
void vm::_update_fc_0() noexcept
{
    // if (use_fc_)
    // {
    //     auto [ done, error ] = fc_.set();
    //     if (!dhresult::check(done, error, [this] { set_op_error("Уставки ПЧ"); }))
    //         return;
    // }
    //
    // next_state(&vm::_update_fc_1);
}

// В зависимости от уставок включаем или выключаем ПЧ
void vm::_update_fc_1() noexcept
{
    if (use_fc_)
    {
        auto [ done, error ] = fc_powered_ ? fc_.turn_on() : fc_.turn_off();
        if (!dhresult::check(done, error, [this] { set_op_error("ПЧ"); }))
            return;
    }

    if (next_target_position_.empty())
    {
        // Отдаем следующую позицию CNC
        next_state(&vm::_cnc_target_move);
    }
    else
    {
        next_target_position_.clear();
        // Либо действуем на опережение
        next_state(&vm::try_add_next_target);
    }
}

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

void vm::stop_program() noexcept
{
    aem::log::trace("vm::stop_program");
    
    // Если на данный момент состояние осей вращения нулевое, 
    // нет смысла останавливать вращение
    auto it = std::find_if(spin_.begin(), spin_.end(), 
        [](auto const &pair) { return std::fpclassify(pair.second) != FP_ZERO; });

    if (it == spin_.end())
        spin_.clear();
    else
        std::for_each(spin_.begin(), spin_.end(), [] (auto &pair) { pair.second = 0.0f; });

    sprayer_id_ = 0;

    if (spin_.empty())
        next_state(&vm::stop_moving);
    else
        next_state(&vm::stop_spin);
}

void vm::stop_spin() noexcept
{
    aem::log::trace("vm::stop_spin");
    
    auto [ done, error ] = cnc_.independent_move(spin_);
    if (!dhresult::check(done, error, [this] { next_state(&vm::stop_moving); }))
        return;
    next_state(&vm::stop_moving);
}

void vm::stop_moving() noexcept
{
    aem::log::trace("vm::stop_moving");
    
    auto [ done, error ] = cnc_.stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&vm::stop_fc); }))
        return;
    next_state(&vm::wait_target_move_stop);
}

void vm::wait_target_move_stop() noexcept
{
    if (cnc_.status() == engine::cnc_status::idle)
    {
        next_state(&vm::stop_fc);
        return;
    }

    if (cnc_.status() != engine::cnc_status::queue)
        return;

    next_state(&vm::reset_queue);
}

void vm::reset_queue() noexcept
{
    auto [ done, error ] = cnc_.hard_stop_target_move();
    if (!dhresult::check(done, error, [this] { next_state(&vm::stop_fc); }))
        return;
    next_state(&vm::wait_target_move_stop);
}

void vm::stop_fc() noexcept
{
    aem::log::trace("vm::stop_fc");
    
    if (use_fc_)
    {
        auto [ done, error ] = fc_.turn_off();
        if (!dhresult::check(done, error, [this] { next_state(use_sprayers_ ? &vm::turn_off_sprayers_pump : &vm::stop_sprayer); }))
            return;
    }
    next_state(use_sprayers_ ? &vm::turn_off_sprayers_pump : &vm::stop_sprayer);
}

void vm::turn_off_sprayers_pump() noexcept
{
    aem::log::warn("---------------vm::turn_off_sprayers_pump");
    hw_.send_event("auto", "start", false);
    next_state(&vm::stop_sprayer);
}

void vm::stop_sprayer() noexcept
{
    aem::log::trace("vm::stop_sprayer");
    
    if (sprayer_id_ >= sprayers_state_.size() || !use_sprayers_)
    {
        next_state(nullptr);
        return;
    }

    auto [ done, error ] = sprayer_[sprayer_id_].set(false);
    if (!dhresult::check(done, error, [this] { sprayer_id_ += 1; next_state(&vm::stop_sprayer); }))
        return;
    sprayer_id_ += 1;
    next_state(&vm::stop_sprayer);
}

