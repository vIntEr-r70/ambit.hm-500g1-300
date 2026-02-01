#pragma once

#include <eng/buffer.hpp>
#include <eng/read-cast.hpp>
#include <eng/utils.hpp>

#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

enum class centering_type : std::uint8_t
{
    not_active,
    tooth_in,
    tooth_out,
    shaft
};

struct program
{
    enum
    {
        fcp_current = 0,
        fcp_power,
        fcp_count
    };

    enum class op_type : std::uint8_t
    {
        main,
        pause,
        loop,
        fc,
        center,
    };

    struct fc_item_t
    {
        union
        {
            struct
            {
                double P;
                double I;
            };

            double fcp[2];
        };

        bool operator ==(fc_item_t const& other) const noexcept
        {
            if (&other == this) return true;
            return
                (std::lround(P * 100) == std::lround(other.P * 100)) &&
                (std::lround(I * 100) == std::lround(other.I * 100));
        }

        void save(eng::buffer::id_t b) const
        {
            eng::buffer::append(b, P);
            eng::buffer::append(b, I);
        }

        void load(std::span<std::byte const> &span)
        {
            P = eng::read_cast<double>(span);
            I = eng::read_cast<double>(span);
        }
    };

    struct main_op
    {
        bool absolute{ false };
        std::vector<fc_item_t> fc;
        std::vector<bool> sprayer;
        std::vector<double> spin;
        struct target_t
        {
            double speed;
            std::vector<double> pos;

            void save(eng::buffer::id_t b) const
            {
                eng::buffer::append(b, speed);
                std::ranges::for_each(pos, [b](double pos) { eng::buffer::append(b, pos); });
            }

            void load(std::span<std::byte const> &span)
            {
                speed = eng::read_cast<double>(span);
                std::ranges::for_each(pos, [&span](double &pos) { pos = eng::read_cast<double>(span); });
            }
        };
        target_t target;

        void save(eng::buffer::id_t b) const
        {
            eng::buffer::append<std::uint8_t>(b, absolute); 

            std::ranges::for_each(fc, [b](auto const &fc) { fc.save(b); });
            std::ranges::for_each(sprayer, [b](bool sp) { eng::buffer::append<std::uint8_t>(b, sp); });
            std::ranges::for_each(spin, [b](double s) { eng::buffer::append(b, s); });

            target.save(b);
        }

        void load(std::span<std::byte const> &span)
        {
            absolute = eng::read_cast<std::uint8_t>(span) != 0;

            std::ranges::for_each(fc, [&span](auto &fc) { fc.load(span); });
            for (std::size_t i = 0; i < sprayer.size(); ++i)
                sprayer[i] = eng::read_cast<std::uint8_t>(span) != 0;
            std::ranges::for_each(spin, [&span](double &s) { s = eng::read_cast<double>(span); });

            target.load(span);
        }
    };

    struct pause_op
    {
        std::uint64_t msec{ 0 };

        void save(eng::buffer::id_t b) const
        {
            eng::buffer::append(b, msec); 
        }

        void load(std::span<std::byte const> &span)
        {
            msec = eng::read_cast<std::uint64_t>(span);
        }
    };

    struct loop_op
    {
        std::uint32_t opid;
        std::uint32_t N;

        void save(eng::buffer::id_t b) const
        {
            eng::buffer::append(b, opid);
            eng::buffer::append(b, N);
        }

        void load(std::span<std::byte const> &span)
        {
            opid = eng::read_cast<std::uint32_t>(span);
            N = eng::read_cast<std::uint32_t>(span);
        }
    };

    struct fc_op
    {
        double p{ 0.0f };
        double i{ 0.0f };
        double tv{ 0.0f };

        void save(eng::buffer::id_t b) const
        {
            eng::buffer::append(b, p);
            eng::buffer::append(b, i);
            eng::buffer::append(b, tv);
        }

        void load(std::span<std::byte const> &span)
        {
            p = eng::read_cast<double>(span);
            i = eng::read_cast<double>(span);
            tv = eng::read_cast<double>(span);
        }
    };

    struct center_op
    {
        centering_type type;
        double shift;

        void save(eng::buffer::id_t b) const
        {
            eng::buffer::append(b, static_cast<std::uint8_t>(type));
            eng::buffer::append(b, shift);
        }

        void load(std::span<std::byte const> &span)
        {
            type = static_cast<centering_type>(eng::read_cast<std::uint8_t>(span));
            shift = eng::read_cast<double>(span);
        }
    };

    std::size_t rows() const noexcept { return phases.size(); }

    void add_default_main_op() noexcept
    {
        main_ops.push_back({
                false,
                std::vector<fc_item_t>(fc_count, fc_item_t{ 0.0f, 0.0f }),
                std::vector<bool>(sprayer_count, false), 
                std::vector<double>(s_axis.size(), 0.0f), 
                { 0.0f, std::vector<double>(t_axis.size(), 0.0f) }
            });
    }

    bool is_axis_is_spin(char axis) const noexcept
    {
        auto it = std::find(s_axis.begin(), s_axis.end(), axis);
        if (it == s_axis.end()) return false;

        std::size_t id = std::distance(s_axis.begin(), it);
        return std::any_of(main_ops.begin(), main_ops.end(), [id](main_op const& op) {
            return std::fpclassify(op.spin[id]) != FP_ZERO;
        });
    }

    // Проверяем корректность использования осей вращения
    // Если хоть где-то в программе ось используется в независимом вращении
    // Ее нельзя задействовать в позиционном перемещеннии
    void modify_using_axis(std::vector<char> &spin_axis, std::vector<char> &target_axis)
    {
        // Исходный позиционный список состоит из осей, отсутствующих в списке независимого движения
        t_axis_m.clear();
        for (std::size_t i = 0; i < t_axis.size(); ++i)
        {
            auto it = std::find(s_axis.begin(), s_axis.end(), t_axis[i]);
            if (it == s_axis.end()) 
                t_axis_m.push_back(i);
        }

        s_axis_m.clear();
        for (std::size_t i = 0; i < s_axis.size(); ++i)
        {
            if (is_axis_is_spin(s_axis[i]))
                s_axis_m.push_back(i);
            else
            {
                auto it = std::find(t_axis.begin(), t_axis.end(), s_axis[i]);
                t_axis_m.push_back(std::distance(t_axis.begin(), it));
            }
        }

        for (auto const& sm : s_axis_m)
            spin_axis.push_back(s_axis[sm]);

        for (auto const& tm : t_axis_m)
            target_axis.push_back(t_axis[tm]);
    }

    void save(eng::buffer::id_t b) const
    {
        eng::buffer::append<std::uint32_t>(b, fc_count);
        eng::buffer::append<std::uint32_t>(b, sprayer_count);

        std::size_t count = s_axis.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(s_axis, [b](char axis) { eng::buffer::append(b, axis); });

        count = t_axis.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(t_axis, [b](char axis) { eng::buffer::append(b, axis); });

        count = main_ops.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(main_ops, [b](main_op const& op) { op.save(b); });

        count = pause_ops.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(pause_ops, [b](pause_op const &sp) { sp.save(b); });

        count = loop_ops.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(loop_ops, [b](loop_op const &sp) { sp.save(b); });

        count = fc_ops.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(fc_ops, [b](fc_op const &sp) { sp.save(b); });

        count = center_ops.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(center_ops, [b](center_op const &sp) { sp.save(b); });

        count = phases.size();
        eng::buffer::append<std::uint32_t>(b, count);
        std::ranges::for_each(phases, [b](op_type sp) {
            eng::buffer::append(b, static_cast<std::uint8_t>(sp));
        });
    }

    bool load(std::span<std::byte const> &span)
    {
        fc_count = eng::read_cast<std::uint32_t>(span);
        sprayer_count = eng::read_cast<std::uint32_t>(span);

        std::size_t count = eng::read_cast<std::uint32_t>(span);
        s_axis.resize(count);

        std::for_each(s_axis.begin(), s_axis.end(), [&span](char &axis) { axis = eng::read_cast<char>(span); });

        count = eng::read_cast<std::uint32_t>(span);
        t_axis.resize(count);
        std::for_each(t_axis.begin(), t_axis.end(), [&span](char &axis) { axis = eng::read_cast<char>(span); });

        count = eng::read_cast<std::uint32_t>(span);
        main_ops.resize(count);
        std::for_each(main_ops.begin(), main_ops.end(), [this, &span](main_op &op)
        { 
            op.fc.resize(fc_count);
            op.sprayer.resize(sprayer_count);
            op.target.pos.resize(t_axis.size());
            op.spin.resize(s_axis.size());

            op.load(span); 
        });

        count = eng::read_cast<std::uint32_t>(span);
        pause_ops.resize(count);
        std::ranges::for_each(pause_ops, [&span](pause_op &sp) { sp.load(span); });

        count = eng::read_cast<std::uint32_t>(span);
        loop_ops.resize(count);
        std::ranges::for_each(loop_ops, [&span](loop_op &sp) { sp.load(span); });

        count = eng::read_cast<std::uint32_t>(span);
        fc_ops.resize(count);
        std::ranges::for_each(fc_ops, [&span](fc_op &sp) { sp.load(span); });

        count = eng::read_cast<std::uint32_t>(span);
        center_ops.resize(count);
        std::ranges::for_each(center_ops, [&span](center_op &sp) { sp.load(span); });

        count = eng::read_cast<std::uint32_t>(span);
        phases.resize(count);
        std::ranges::for_each(phases, [&span](op_type &sp) {
            sp = static_cast<op_type>(eng::read_cast<std::uint8_t>(span));
        });

        return true;
    }

    void for_each(auto &&f) const
    {
        std::size_t main_op_id = 0;
        std::size_t pause_op_id = 0;
        std::size_t loop_op_id = 0;
        std::size_t fc_op_id = 0;
        std::size_t center_op_id = 0;

        for (auto &&type : phases)
        {
            switch(type)
            {
            case op_type::main:
                f(type, main_op_id++);
                break;
            case op_type::pause:
                f(type, pause_op_id++);
                break;
            case op_type::loop:
                f(type, loop_op_id++);
                break;
            case op_type::fc:
                f(type, fc_op_id++);
                break;
            case op_type::center:
                f(type, center_op_id++);
                break;
            }
        }
    }

    std::pair<op_type, std::size_t> op_info(std::size_t op_id) const noexcept
    {
        auto rtype = phases[op_id];
        return { rtype, get_rid(rtype, op_id) };
    }

    std::size_t get_rid(op_type rtype, std::size_t op_id) const noexcept
    {
        std::size_t rid = 0;
        std::for_each(phases.begin(), phases.begin() + op_id, 
            [&rid, rtype] (auto type) { rid += (type == rtype) ? 1 : 0; });
        return rid;
    }

    void for_fc(std::size_t rid, auto &&f) const
    {
        auto const& op = main_ops[rid];
        for (std::size_t i = 0; i < fc_count; ++i)
            f(i, op.fc[i]);
    }

    void for_sprayer(std::size_t rid, auto &&f) const
    {
        auto const& op = main_ops[rid];
        for (std::size_t i = 0; i < sprayer_count; ++i)
            f(i, op.sprayer[i]);
    }

    void for_spin_axis(std::size_t rid, auto &&f) const
    {
        auto const& op = main_ops[rid];
        for (auto const& id : s_axis_m)
            f(s_axis[id], op.spin[id]);
    }

    void for_target_axis(std::size_t rid, auto &&f) const
    {
        auto const& op = main_ops[rid];
        for (auto const& id : t_axis_m)
            f(t_axis[id], op.target.pos[id]);
    }

    void clear() noexcept
    {
        main_ops.clear();
        pause_ops.clear();
        loop_ops.clear();
        fc_ops.clear(); 
        center_ops.clear(); 

        phases.clear();
    }

    bool is_spin_axis(char axis) const noexcept {
        return std::find(s_axis.begin(), s_axis.end(), axis) != s_axis.end();
    }

    bool is_target_spin_axis(std::size_t id) const noexcept {
        return is_spin_axis(t_axis.at(id));
    }

    std::size_t fc_count{ 0 };
    std::size_t sprayer_count{ 0 };

    std::vector<char> s_axis;
    std::vector<char> t_axis;

    std::vector<std::size_t> s_axis_m;
    std::vector<std::size_t> t_axis_m;

    std::vector<main_op> main_ops;
    std::vector<pause_op> pause_ops;
    std::vector<loop_op> loop_ops;
    std::vector<fc_op> fc_ops; 
    std::vector<center_op> center_ops; 

    std::vector<op_type> phases;
};

