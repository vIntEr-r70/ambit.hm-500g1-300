#pragma once

#include <eng/sibus/node.hpp>
#include <eng/timer.hpp>
#include <eng/stopwatch.hpp>

class barrel_lvl_ctl final
    : public eng::sibus::node
{
    eng::sibus::output_port_id_t valve_;
    eng::sibus::input_wire_id_t ictl_;

    double refilled_volume_;
    double target_volume_;

    double flow_rate_{ NAN };

    eng::timer::id_t tid_;

    eng::stopwatch stopwatch_;

public:

    barrel_lvl_ctl(std::string_view);

private:

    void update_flow_rate(double);

    void start_refilling(double);

    void stop_refilling();

    void decrease_amount(double);

private:

    bool is_flow_sensor_first_init(double) const noexcept;
};

