#pragma once

#include <eng/json.hpp>
#include <eng/json/builder.hpp>

#include <string>
#include <cstdint>

namespace axis_config
{

    struct desc
    {
        std::string name;

        bool allow_in_auto{ false };

        double length{ 0.0 };

        // Определяет какой из концевиков считается домом
        // false = MIN, true = MAX
        bool home{ false };

        bool min_limit_inversion{ false };
        bool max_limit_inversion{ false };

        double ratio{ 1.0 };

        //! Скорость задается в мм/мин или рад/мин
        //! Ускорение в мм/мин2 либо рад/мин2
        //! Коэфициент в шаг/мм либо шаг/рад
        //! Именно с этими единицами работает плата Курина
        double acc{ 1.0 };

        //! [мм/мин] [рад/мин]
        double speed[3] { 1.0, 10.0, 100.0 };

        struct
        {
            double ratio{ 1.0 };

            //! [мм/мин] [рад/мин]
            double speed[3] { 1.0, 10.0, 100.0 };

            char link_axis{ '\0' };

        } rcu;

        bool get_home_limit(bool min, bool max) const noexcept { return home ? max : min; }
    };

    // Загружаем из JSON
    constexpr static void load(desc &axis, std::string_view json)
    {
        eng::json::object obj(json);

        axis.name = obj.get_sv("name");
        axis.allow_in_auto = obj.get<bool>("allow-in-auto");
        axis.length = obj.get<double>("length");
        axis.home = obj.get<bool>("home");
        axis.min_limit_inversion = obj.get<bool>("min-limit-inversion");
        axis.max_limit_inversion = obj.get<bool>("max-limit-inversion");
        axis.ratio = obj.get<double>("ratio");
        axis.acc = obj.get<double>("acc");

        obj.get_array("speed").for_each(
            [&axis](std::size_t idx, eng::json::value v) {
                axis.speed[idx] = v.get<double>();
            });

        eng::json::object rcu{ obj.get_object("rcu") };

        axis.rcu.ratio = rcu.get<double>("ratio");
        axis.rcu.link_axis = rcu.get<char>("link-axis");

        rcu.get_array("speed").for_each(
            [&axis](std::size_t idx, eng::json::value v) {
                axis.rcu.speed[idx] = v.get<double>();
            });
    }

    // Сохраняем в JSON
    constexpr static void save(desc const &axis, eng::json::builder_t &jb)
    {
        eng::json::add_object(jb, [&axis](auto &jb)
        {
            eng::json::add_key_value(jb, "name", [&axis](auto &jb) {
                eng::json::add_string(jb, axis.name);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "allow-in-auto", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.allow_in_auto);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "length", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.allow_in_auto);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "home", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.home);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "min-limit-inversion", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.min_limit_inversion);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "max-limit-inversion", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.max_limit_inversion);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "ratio", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.ratio);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "acc", [&axis](auto &jb) {
                eng::json::add_value(jb, axis.acc);
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "speed", [&axis](auto &jb)
            {
                eng::json::add_array(jb, [&axis](auto &jb)
                {
                    eng::json::add_value(jb, axis.speed[0]);
                    eng::json::add_comma(jb);
                    eng::json::add_value(jb, axis.speed[1]);
                    eng::json::add_comma(jb);
                    eng::json::add_value(jb, axis.speed[2]);
                });
            });

            eng::json::add_comma(jb);

            eng::json::add_key_value(jb, "rcu", [&axis](auto &jb)
            {
                eng::json::add_object(jb, [&axis](auto &jb)
                {
                    eng::json::add_key_value(jb, "ratio", [&axis](auto &jb) {
                        eng::json::add_value(jb, axis.rcu.ratio);
                    });

                    eng::json::add_comma(jb);

                    eng::json::add_key_value(jb, "link-axis", [&axis](auto &jb) {
                        eng::json::add_value(jb, axis.rcu.link_axis);
                    });

                    eng::json::add_comma(jb);

                    eng::json::add_key_value(jb, "speed", [&axis](auto &jb)
                    {
                        eng::json::add_array(jb, [&axis](auto &jb)
                        {
                            eng::json::add_value(jb, axis.rcu.speed[0]);
                            eng::json::add_comma(jb);
                            eng::json::add_value(jb, axis.rcu.speed[1]);
                            eng::json::add_comma(jb);
                            eng::json::add_value(jb, axis.rcu.speed[2]);
                        });
                    });
                });
            });

        });
    }

}

