#include "program-model.hpp"
#include "ProgramModelHeader.h"

#include <QColor>

#include <eng/log.hpp>

static QString const& remove_final_zeros(QString &value)
{
    int pos = value.indexOf('.');
    if (pos < 0) return value;

    while (value.length())
    {
        QChar const symbol = value.back();
        if (symbol == '0' || symbol == '.')
        {
            value.chop(1);

            if (symbol != '.')
                continue;
        }

        break;
    }

    return value;
}

void program_model::set_program(program value)
{
    layoutAboutToBeChanged();
        program_ = std::move(value);
        current_row_ = program_.rows();
    layoutChanged(); 
}

int program_model::rowCount(QModelIndex const&) const
{
    return program_.rows();
}

int program_model::columnCount(QModelIndex const&) const
{
    return ProgramModelHeader::column_count(program_);
}

QVariant program_model::data(QModelIndex const& index, int role) const
{
    if (role == Qt::TextAlignmentRole)
        return Qt::AlignCenter;

    std::size_t const row = index.row();
    std::size_t const col = index.column();

    auto [type, rid] = program_.op_info(row);

    switch (role)
    {
    case Qt::ForegroundRole:
        return QColor(Qt::blue);

    case Qt::DisplayRole:
        if (col == 0)
#ifdef BUILDROOT
            return QString::number(row + 1);
#else
            return QString::number(row);
#endif
        break;

    case Qt::BackgroundRole:
        if (col == 0)
            return (current_row_ == row) ? QColor("#ffb800") : QVariant();
        break;

    default:
        return QVariant();
    }

    if (role == Qt::BackgroundRole)
    {
        if (type != program::op_type::main)
        {
            switch(type)
            {
            case program::op_type::pause:
                return program_.pause_ops.at(rid).msec ? QColor("#98FB98") : QColor("#FA8072");
            case program::op_type::loop:
                return QColor("#87CEEB");
            case program::op_type::fc:
                return QColor("#ffd700");
            case program::op_type::center:
                return QColor("#8b90ff");
            default:
                return QVariant();
            }
        }
        else
        {
            return data_main_op_equal(rid, col) ? QColor(Qt::gray) : QColor(Qt::white);
        }
    }
    else
    {
        if (type != program::op_type::main)
        {
            switch(type)
            {
            case program::op_type::pause: {
                auto msec = program_.pause_ops.at(rid).msec;
                auto s = msec / 1000;
                char buf[64];
                std::snprintf(buf, sizeof(buf), "Пауза [ %02zu:%02zu:%02zu.%01zu ]",
                        s / 3600, (s % 3600) / 60, s % 60, (msec % 1000) / 100);
                return QString(buf); }

            case program::op_type::fc: {
                auto const& op = program_.fc_ops.at(rid);
                char buf[128];
                std::snprintf(buf, sizeof(buf), "Прецизионный нагрев [ Мощность: %.2f кВт, Ток: %.2f А, Время: %.1f секунд ]", op.p / 1000.0, op.i, op.tv);
                return QString(buf); }

            case program::op_type::loop: {
                auto const& op = program_.loop_ops.at(rid);
                char buf[64];
                std::snprintf(buf, sizeof(buf), "Переход на [ %u ] N=%zu", op.opid + 1, loop_repeat_count(row, op.N));
                return QString(buf); }

            case program::op_type::center: {
                auto const& op = program_.center_ops.at(rid);
                char buf[128];
                if (op.type != centering_type::shaft)
                {
                    char const* type = (op.type == centering_type::tooth_in) ? "внутреннего" : "внешнего";
                    std::snprintf(buf, sizeof(buf), "Поиск центра %s зуба, h = %.1f", type, op.shift);
                }
                else
                {
                    std::snprintf(buf, sizeof(buf), "Поиск центра вала");
                }
                return QString(buf); }

            default:
                return QVariant();
            }
        }
        else
        {
            QString value = data_main_op_text(rid, col);
            return remove_final_zeros(value);
        }
    }
}

bool program_model::data_main_op_equal(std::size_t rid, std::size_t col) const
{
    if (rid == 0)
        return false;

    auto const& op = program_.main_ops.at(rid);
    auto const& prev = program_.main_ops.at(rid - 1);

    auto [type, id] = ProgramModelHeader::cell(program_, col);

    switch(type)
    {
    case ProgramModelHeader::FcI:
        return (op.fc[id].I == prev.fc[id].I);
    case ProgramModelHeader::FcP:
        return (op.fc[id].P == prev.fc[id].P);
    case ProgramModelHeader::Sprayer:
        return (op.sprayer[id] == prev.sprayer[id]);
    case ProgramModelHeader::Spin:
        return (op.spin[id] == prev.spin[id]);
    case ProgramModelHeader::TargetPos:
        return (op.target.pos[id] == prev.target.pos[id]);
    case ProgramModelHeader::TargetSpeed: 
        return (op.target.speed == prev.target.speed);
    }

    return false;
}

QString program_model::data_main_op_text(std::size_t rid, std::size_t col) const
{
    auto const& op = program_.main_ops.at(rid);

    auto [type, id] = ProgramModelHeader::cell(program_, col);

    switch(type)
    {
    case ProgramModelHeader::FcI:
        return QString::number(op.fc[id].I, 'f', 2);
    case ProgramModelHeader::FcP:
        return QString::number(op.fc[id].P / 1000.0, 'f', 2);
    case ProgramModelHeader::Sprayer:
        return QString(op.sprayer[id] ? "ВКЛ" : "ВЫКЛ");
    case ProgramModelHeader::Spin:
        return QString::number(op.spin[id] / 6, 'f', 2);
    case ProgramModelHeader::TargetPos: {
        bool spin = program_.is_target_spin_axis(id);
        return QString(op.absolute ? "" : "Δ ") + QString::number(op.target.pos[id], 'f', spin ? 4 : 2); }
    case ProgramModelHeader::TargetSpeed:
        if (id == 0) return QString::number(op.target.speed, 'f', 2);
        return QString::number(op.target.speed / 6, 'f', 2);
    }

    return "";
}

