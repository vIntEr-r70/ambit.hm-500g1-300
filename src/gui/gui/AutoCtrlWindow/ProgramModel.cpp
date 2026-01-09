#include "ProgramModel.h"
#include "ProgramModelHeader.h"
#include "UnitsCalc.h"

#include <QColor>
#include <QFile>

#include <aem/utils/crc.h>
#include <aem/utils/base64.h>
#include <aem/log.h>
#include <aem/environment.h>

ProgramModel::ProgramModel()
    : QAbstractTableModel()
    , path_(aem::getenv("LIAEM_RW_PATH", "./home-root"))
{
    path_ /= "programs";
    reset();
}

void ProgramModel::change_sprayer(std::size_t rid, std::size_t id) noexcept
{
    auto &sprayer = program_.main_ops[rid].sprayer;
    sprayer[id] = !sprayer[id];
    dataChanged(createIndex(current_row_, 0), createIndex(current_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_main(std::size_t ctype, std::size_t rid, std::size_t id, float value) noexcept
{
    auto &op = program_.main_ops[rid];

    switch(ctype)
    {
    // Храним в Ватах
    case ProgramModelHeader::FcP:
        op.fc[id].P = value * 1000;
        break;
    // Храним в Амперах
    case ProgramModelHeader::FcI:
        op.fc[id].I = value;
        break;

    // Приходят в об/мин, программа хранит в рад/мин
    case ProgramModelHeader::Spin:
        op.spin[id] = UnitsCalc::fromSpeed(true, value);
        break;

    // Приходят в мм или град, программа хранит в мм или рад
    case ProgramModelHeader::TargetPos:
        op.target.pos[id] = UnitsCalc::fromPos(program_.is_target_spin_axis(id), value);
        break;

    // Приходят в мм/с или об/мин, программа хранит в мм/мин или рад/мин
    case ProgramModelHeader::TargetSpeed: {
        op.target.set_in = static_cast<program::tspeed_t>(id);
        bool spin = op.target.set_in == program::tspeed_t::rev_min;
        op.target.speed = UnitsCalc::fromSpeed(spin, value);
        break; }

    default:
        return;
    }

    dataChanged(createIndex(current_row_, 0), createIndex(current_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_pause(std::size_t rid, aem::uint64 v) noexcept
{
    auto &op = program_.pause_ops[rid];
    op.msec = v;
    dataChanged(createIndex(current_row_, 0), createIndex(current_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_loop(std::size_t rid, std::size_t opid, std::size_t N) noexcept
{
    auto &op = program_.loop_ops[rid];
    op.opid = opid;
    op.N = N;
    dataChanged(createIndex(current_row_, 0), createIndex(current_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_fc(std::size_t rid, float p, float i, float sec) noexcept
{
    auto &op = program_.fc_ops[rid];
    op.p = p * 1000;
    op.i = i;
    op.tv = sec;
    dataChanged(createIndex(current_row_, 0), createIndex(current_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_center(std::size_t rid, centering_type type, float shift) noexcept
{
    auto &op = program_.center_ops[rid];
    op.type = type;
    op.shift = UnitsCalc::fromPos(false, shift);
    dataChanged(createIndex(current_row_, 0), createIndex(current_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::set_current_row(std::size_t row) noexcept
{
    if (row == current_row_)
    {
        if (row == aem::MaxSizeT)
            return;
        row = aem::MaxSizeT;
    }

    std::size_t cc = ProgramModelHeader::column_count(program_);

    if (current_row_ != aem::MaxSizeT)
    {
        current_row_ = aem::MaxSizeT;
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, cc - 1));
    }

    current_row_ = row;

    if (current_row_ != aem::MaxSizeT)
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, cc - 1));
}

void ProgramModel::set_current_phase(std::size_t id) noexcept
{
    if (current_row_ == id)
        current_row_ = aem::MaxSizeT;
    set_current_row(id);
}

// Добавляем новую операцию, если это перавая то все по дефолту, 
// если уже есть операции то копия последней
void ProgramModel::add_op(program::op_type type) noexcept
{
    std::size_t irow = (current_row_ == aem::MaxSizeT) ? program_.phases.size() : current_row_;
    auto rid = program_.get_rid(type, irow);

    if (type == program::op_type::pause)
    {
        auto &oplist = program_.pause_ops;
        if (oplist.empty())
            program_.pause_ops.push_back({ 0 });
        else
            oplist.insert(oplist.begin() + rid, oplist[(rid == oplist.size()) ? (rid - 1) : rid]);
    }
    else if (type == program::op_type::loop)
    {
        auto &oplist = program_.loop_ops;
        if (oplist.empty())
            program_.loop_ops.push_back({ 0, 1 });
        else
            oplist.insert(oplist.begin() + rid, oplist[(rid == oplist.size()) ? (rid - 1) : rid]);
    }
    else if (type == program::op_type::fc)
    {
        auto &oplist = program_.fc_ops;
        if (oplist.empty())
            program_.fc_ops.push_back({ 0, 0, 0 });
        else
            oplist.insert(oplist.begin() + rid, oplist[(rid == oplist.size()) ? (rid - 1) : rid]);
    }
    else if (type == program::op_type::center)
    {
        auto &oplist = program_.center_ops;
        if (oplist.empty())
            program_.center_ops.push_back({ centering_type::shaft, 0 });
        else
            oplist.insert(oplist.begin() + rid, oplist[(rid == oplist.size()) ? (rid - 1) : rid]);
    }
    else
    {
        return;
    }

    beginInsertRows(QModelIndex(), irow, irow);
    program_.phases.insert(program_.phases.begin() + irow, type);
    endInsertRows();

    // table_.setSpan(irow, 1, 1, ProgramModelHeader::column_count(program_) - 1);
}

void ProgramModel::add_main_op(bool absolute) noexcept
{
    std::size_t irow = (current_row_ == aem::MaxSizeT) ? program_.phases.size() : current_row_;
    auto rid = program_.get_rid(program::op_type::main, irow);
    auto &oplist = program_.main_ops;

    if (oplist.empty())
        program_.add_default_main_op();
    else
        oplist.insert(oplist.begin() + rid, oplist[(rid == oplist.size()) ? (rid - 1) : rid]);

    oplist[rid].absolute = absolute;

    beginInsertRows(QModelIndex(), irow, irow);
    program_.phases.insert(program_.phases.begin() + irow, program::op_type::main);
    endInsertRows();
}

void ProgramModel::remove_op() noexcept
{
    if (current_row_ == aem::MaxSizeT || current_row_ >= program_.rows())
        return;

    auto [type, rid] = program_.op_info(current_row_);

    if (type == program::op_type::main)
    {
        auto &oplist = program_.main_ops;
        oplist.erase(oplist.begin() + rid);
    }
    else if (type == program::op_type::pause)
    {
        auto &oplist = program_.pause_ops;
        oplist.erase(oplist.begin() + rid);
    }
    else if (type == program::op_type::loop)
    {
        auto &oplist = program_.loop_ops;
        oplist.erase(oplist.begin() + rid);
    }
    else if (type == program::op_type::fc)
    {
        auto &oplist = program_.fc_ops;
        oplist.erase(oplist.begin() + rid);
    }
    else if (type == program::op_type::center)
    {
        auto &oplist = program_.center_ops;
        oplist.erase(oplist.begin() + rid);
    }
    else
    {
        return;
    }

    auto &plist = program_.phases;
    plist.erase(plist.begin() + current_row_);

    beginRemoveRows(QModelIndex(), current_row_, current_row_);
    endRemoveRows();

    if (current_row_ >= program_.rows())
    {
        if (current_row_ == 0)
            current_row_ = aem::MaxSizeT;
        else
            current_row_ -= 1;

        std::size_t cc = ProgramModelHeader::column_count(program_);
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, cc - 1));
    }
}

int ProgramModel::rowCount(QModelIndex const&) const
{
    return program_.rows();
}

int ProgramModel::columnCount(QModelIndex const&) const 
{ 
    return ProgramModelHeader::column_count(program_); 
}

QVariant ProgramModel::data(QModelIndex const& index, int role) const
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
        {
            QString result = QString::number(row + 1);
            if (program_.phases[row] == program::op_type::main)
                if (!program_.main_ops.at(rid).absolute)
                    result = "Δ" + result;
            return result;
        }
        break;

    case Qt::BackgroundRole:
        if (col == 0)
            return (row == current_row_) ? QColor("#ffb800") : QVariant();
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
                std::snprintf(buf, sizeof(buf), "Пауза [ %02u:%02u:%02u.%01u ]",
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
                std::snprintf(buf, sizeof(buf), "Переход на [ %u ] N=%u", op.opid + 1, op.N);
                return QString(buf); }

            case program::op_type::center: {
                auto const& op = program_.center_ops.at(rid);
                char buf[128];
                if (op.type != centering_type::shaft)
                {
                    char const* type = (op.type == centering_type::tooth_in) ? "внутреннего" : "внешнего";
                    std::snprintf(buf, sizeof(buf), "Поиск центра %s зуба, h = %.1f", type, UnitsCalc::toPos(false, op.shift));
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
            QString value(data_main_op_text(rid, col));
            // for (std::size_t i = 0; i < prec_ + 1; ++i)
            // {
            //     if (tv.length() && (tv.back() == '0' || tv.back() == '.' || tv.back() == ','))
            //         tv.remove(tv.length() - 1, 1);
            //     else
            //         break;
            // }
            return value;
        }
    }
}

bool ProgramModel::data_main_op_equal(std::size_t rid, std::size_t col) const
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
        return (op.target.speed == prev.target.speed && 
                op.target.set_in == prev.target.set_in); 
    }

    return false;
}

QString ProgramModel::data_main_op_text(std::size_t rid, std::size_t col) const
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
        return QString::number(UnitsCalc::toSpeed(true, op.spin[id]), 'f', 2);
    case ProgramModelHeader::TargetPos:
        return QString::number(UnitsCalc::toPos(program_.is_target_spin_axis(id), op.target.pos[id]), 'f', 2);
    case ProgramModelHeader::TargetSpeed: {
        bool spin = op.target.set_in == program::tspeed_t::rev_min;
        if (static_cast<program::tspeed_t>(id) == op.target.set_in)
            return QString::number(UnitsCalc::toSpeed(spin, op.target.speed), 'f', 2);
        return QString::number(UnitsCalc::toSpeed(!spin, op.target.speed), 'f', 2); }
    }

    return "";
}

std::string ProgramModel::get_base64_program() const noexcept
{
    aem::buffer buf;
    program_.save(buf);
    return aem::utils::base64::encode(buf.cbegin(), buf.size());
}

void ProgramModel::reset() noexcept
{
    name_ = "";
    comments_ = "";
    current_row_ = aem::MaxSizeT;

    if (program_.rows())
    {
        beginRemoveRows(QModelIndex(), 0, program_.rows() - 1);
        endRemoveRows();
    }

    program_.clear();

    program_.fc_count = 1;
    program_.sprayer_count = 3;
    program_.s_axis = { 'U', 'V' };
    program_.t_axis = { 'X', 'Y', 'Z', 'U', 'V' };
}

bool ProgramModel::save_to_file() const noexcept
{
    if (!std::filesystem::exists(path_))
        std::filesystem::create_directories(path_);

    if (name_.isEmpty())
        return false;

    QString fname((path_ / name_.toUtf8().constData()).c_str());

    aem::buffer buf;

    std::string comm(comments_.toUtf8().constData());
    buf.append<aem::uint32>(comm.length());
    buf.append(comm);

    program_.save(buf);

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    aem::uint32 crc = aem::utils::crc::crc32(buf.begin(), buf.end());
    int ret = file.write(reinterpret_cast<char const*>(&crc), sizeof(crc));
    if (ret != sizeof(crc))
        return false;

    ret = file.write(buf.c_str(), buf.size());
    if (ret != buf.size())
        return false;

    return true;
}

bool ProgramModel::load_from_file(QString const& fname) noexcept
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data(file.readAll());
    if (data.size() < sizeof(aem::uint32))
        return false;

    aem::buffer buf;
    buf.append(data.data(), data.size());

    aem::uint32 crc = buf.read_cast<aem::uint32>();
    aem::uint32 fcrc = aem::utils::crc::crc32(buf.begin(), buf.end());

    if (crc != fcrc)
        return false;

    reset();

    aem::uint32 slen = buf.read_cast<aem::uint32>();
    comments_ = QString(QByteArray(buf.c_str(), slen));
    buf.drain(slen);

    program tp;
    tp.load(buf);

    if (tp.rows())
    {
        beginInsertRows(QModelIndex(), 0, tp.rows() - 1);
        program_ = std::move(tp);
        endInsertRows();
    }

    // std::size_t i = 0;
    // for (auto const& type : program_.phases)
    // {
    //     if (type != program::op_type::main)
    //         table_.setSpan(i, 1, 1, ProgramModelHeader::column_count(program_) - 1);
    //     i += 1;
    // }

    return true;
}

bool ProgramModel::load_from_usb_file(QString const& fname) noexcept
{
    return false;
}

bool ProgramModel::load_from_local_file(QString name) noexcept
{
    if (!load_from_file((path_ / name.toUtf8().constData()).c_str()))
    {
        aem::log::error("ProgramModel::load_from_local_file: {}", name.toUtf8().constData());
        return false;
    }

    name_ = name;

    return true;
}
