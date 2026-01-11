#pragma once

#include "Defs.h"

#include <aem/buffer.h>
#include <aem/types.h>
#include <aem/log.h>

#include <cmath>

struct program
{
    enum
    {
        fcp_current = 0,
        fcp_power,
        fcp_count
    };

    enum class op_type : aem::uint8
    {
        main,
        pause,
        loop,
        fc,
        center,
    };

    enum class tspeed_t : aem::uint8
    {
        mm_sec,
        rev_min
    };

    struct fc_item_t
    {
        union
        {
            struct
            {
                float P;
                float I;
            };

            float fcp[2];
        };

        bool operator ==(fc_item_t const& other) const noexcept
        {
            if (&other == this) return true;
            return
                (std::lround(P * 100) == std::lround(other.P * 100)) &&
                (std::lround(I * 100) == std::lround(other.I * 100));
        }

        void save(aem::buffer &b) const
        {
            b.append(P);
            b.append(I);
        }

        void load(aem::buffer &b)
        {
            P = b.read_cast<float>();
            I = b.read_cast<float>();
        }
    };

    struct main_op
    {
        bool absolute{ false };
        std::vector<fc_item_t> fc;
        std::vector<bool> sprayer;
        std::vector<float> spin;
        struct target_t
        {
            float speed;
            tspeed_t set_in;
            std::vector<float> pos;

            void save(aem::buffer &b) const
            {
                b.append(speed);
                b.append(static_cast<aem::uint8>(set_in));
                std::for_each(pos.begin(), pos.end(), [&b](float pos) { b.append(pos); });
            }

            void load(aem::buffer &b)
            {
                speed = b.read_cast<float>();
                set_in = static_cast<tspeed_t>(b.read_cast<aem::uint8>());
                std::for_each(pos.begin(), pos.end(), [&b](float &pos) { pos = b.read_cast<float>(); });
            }
        };
        target_t target;

        void save(aem::buffer &b) const
        {
            b.append<aem::uint8>(absolute); 

            std::for_each(fc.begin(), fc.end(), [&b](auto const &fc) { fc.save(b); });
            std::for_each(sprayer.begin(), sprayer.end(), [&b](bool sp) { b.append<aem::uint8>(sp); });
            std::for_each(spin.begin(), spin.end(), [&b](float s) { b.append(s); });

            target.save(b);
        }

        void load(aem::buffer &b)
        {
            absolute = b.read_cast<aem::uint8>() != 0;

            std::for_each(fc.begin(), fc.end(), [&b](auto &fc) { fc.load(b); });
            for (std::size_t i = 0; i < sprayer.size(); ++i)
                sprayer[i] = b.read_cast<aem::uint8>() != 0;
            std::for_each(spin.begin(), spin.end(), [&b](float &s) { s = b.read_cast<float>(); });

            target.load(b);
        }
    };

    struct pause_op
    {
        aem::uint64 msec{ 0 };

        void save(aem::buffer &b) const
        {
            b.append(msec); 
        }

        void load(aem::buffer &b)
        {
            msec = b.read_cast<aem::uint64>();
        }
    };

    struct loop_op
    {
        aem::uint32 opid;
        aem::uint32 N;

        void save(aem::buffer &b) const
        {
            b.append(opid);
            b.append(N);
        }

        void load(aem::buffer &b)
        {
            opid = b.read_cast<aem::uint32>();
            N = b.read_cast<aem::uint32>();
        }
    };

    struct fc_op
    {
        float p{ 0.0f };
        float i{ 0.0f };
        float tv{ 0.0f };

        void save(aem::buffer &b) const
        {
            b.append(p);
            b.append(i);
            b.append(tv);
        }

        void load(aem::buffer &b)
        {
            p = b.read_cast<float>();
            i = b.read_cast<float>();
            tv = b.read_cast<float>();
        }
    };

    struct center_op
    {
        centering_type type;
        float shift;

        void save(aem::buffer &b) const
        {
            b.append(static_cast<aem::uint8>(type));
            b.append(shift);
        }

        void load(aem::buffer &b)
        {
            type = static_cast<centering_type>(b.read_cast<aem::uint8>());
            shift = b.read_cast<float>();
        }
    };

    std::size_t rows() const noexcept { return phases.size(); }

    void add_default_main_op() noexcept
    {
        main_ops.push_back({
                false,
                std::vector<fc_item_t>(fc_count, fc_item_t{ 0.0f, 0.0f }),
                std::vector<bool>(sprayer_count, false), 
                std::vector<float>(s_axis.size(), 0.0f), 
                { 0.0f, tspeed_t::mm_sec, std::vector<float>(t_axis.size(), 0.0f) }
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

    void save(aem::buffer &b) const
    {
        b.append<aem::uint32>(fc_count);
        b.append<aem::uint32>(sprayer_count);

        std::size_t count = s_axis.size();
        b.append<aem::uint32>(count);
        std::for_each(s_axis.begin(), s_axis.end(), [&b](char axis) { b.append(axis); });

        count = t_axis.size();
        b.append<aem::uint32>(count);
        std::for_each(t_axis.begin(), t_axis.end(), [&b](char axis) { b.append(axis); });

        count = main_ops.size();
        b.append<aem::uint32>(count);
        std::for_each(main_ops.begin(), main_ops.end(), [&b](main_op const& op) { op.save(b); });

        count = pause_ops.size();
        b.append<aem::uint32>(count);
        std::for_each(pause_ops.begin(), pause_ops.end(), [&b](pause_op const &sp) { sp.save(b); });

        count = loop_ops.size();
        b.append<aem::uint32>(count);
        std::for_each(loop_ops.begin(), loop_ops.end(), [&b](loop_op const &sp) { sp.save(b); });

        count = fc_ops.size();
        b.append<aem::uint32>(count);
        std::for_each(fc_ops.begin(), fc_ops.end(), [&b](fc_op const &sp) { sp.save(b); });

        count = center_ops.size();
        b.append<aem::uint32>(count);
        std::for_each(center_ops.begin(), center_ops.end(), [&b](center_op const &sp) { sp.save(b); });

        count = phases.size();
        b.append<aem::uint32>(count);
        std::for_each(phases.begin(), phases.end(), [&b](op_type sp) { 
            b.append(static_cast<aem::uint8>(sp)); 
        });
    }

    bool load(aem::buffer &b)
    {
        fc_count = b.read_cast<aem::uint32>();
        sprayer_count = b.read_cast<aem::uint32>();

        std::size_t count = b.read_cast<aem::uint32>();
        s_axis.resize(count);
        std::for_each(s_axis.begin(), s_axis.end(), [&b](char &axis) { axis = b.read_cast<char>(); });

        count = b.read_cast<aem::uint32>();
        t_axis.resize(count);
        std::for_each(t_axis.begin(), t_axis.end(), [&b](char &axis) { axis = b.read_cast<char>(); });

        count = b.read_cast<aem::uint32>();
        main_ops.resize(count);
        std::for_each(main_ops.begin(), main_ops.end(), [this, &b](main_op &op)
        { 
            op.fc.resize(fc_count);
            op.sprayer.resize(sprayer_count);
            op.target.pos.resize(t_axis.size());
            op.spin.resize(s_axis.size());

            op.load(b); 
        });

        count = b.read_cast<aem::uint32>();
        pause_ops.resize(count);
        std::for_each(pause_ops.begin(), pause_ops.end(), [&b](pause_op &sp) { sp.load(b); });

        count = b.read_cast<aem::uint32>();
        loop_ops.resize(count);
        std::for_each(loop_ops.begin(), loop_ops.end(), [&b](loop_op &sp) { sp.load(b); });

        count = b.read_cast<aem::uint32>();
        fc_ops.resize(count);
        std::for_each(fc_ops.begin(), fc_ops.end(), [&b](fc_op &sp) { sp.load(b); });

        count = b.read_cast<aem::uint32>();
        center_ops.resize(count);
        std::for_each(center_ops.begin(), center_ops.end(), [&b](center_op &sp) { sp.load(b); });

        count = b.read_cast<aem::uint32>();
        phases.resize(count);
        std::for_each(phases.begin(), phases.end(), [&b](op_type &sp)
        { 
            sp = static_cast<op_type>(b.read_cast<aem::uint8>());
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
