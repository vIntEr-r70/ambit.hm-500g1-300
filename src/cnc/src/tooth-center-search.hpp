#pragma once

#include <eng/sibus/node.hpp>

class tooth_center_search final
    : public eng::sibus::node
{
    void(tooth_center_search::*step_)();

    // struct axis
    // {
    //     constexpr static std::size_t step{ 0 };
    //     constexpr static std::size_t deep{ 1 };
    // };
    //
    // eng::sibus::output_port_id_t port_step_speed_;
    // eng::sibus::output_port_id_t port_step_position_;
    // eng::sibus::output_port_id_t port_deep_speed_;
    // eng::sibus::output_port_id_t port_deep_position_;
    //
    // // Координата первого торца зуба 
    // double initial_step_position_;
    //
    // // Расстояние от впадины до рабочего положения индуктора
    // double config_deep_offset_;
    //
    // struct axis_t
    // {
    //     bool ready;
    //     double position;
    //     eng::sibus::task_user_id_t task_user_id;
    //     eng::sibus::task_session_id_t task_session_id;
    // };
    // std::array<axis_t, 2> axis_;
    //
    // void(tooth_center_search::*expected_state_)();

    eng::sibus::input_wire_id_t iwire_;

    eng::sibus::output_wire_id_t owire_bki_;

    // Если глубина отрицательная значит зуб внешний
    // если положительная то внутреннии
    double deep_;
    double x_zero_pos_; 

    bool wait_for_active_x_;
    bool wait_for_active_y_;

    struct axis_t
    {
        std::string status;
        bool bki;
        double position;
        double speed;
        eng::sibus::output_wire_id_t owire;
    };

    axis_t x_;
    axis_t y_;

public:

    tooth_center_search();

private:

    void register_on_bus_done() override final;

private:

    void capture_axis();

    void activate_axis_done();

    void move_x_point_1_done();

    void bki_x_point_1_reset();

    void move_x_point_0_done();

    void move_x_point_2_done();

    void move_x_center_done();

    void bki_x_center_reset();

    void move_y_deep_done();

    void move_y_target_done();

    void bki_y_target_reset();

    void deactivate_axis_done();
};
