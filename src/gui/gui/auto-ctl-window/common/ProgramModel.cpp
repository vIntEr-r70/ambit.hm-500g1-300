#include "ProgramModel.h"
#include "ProgramModelHeader.h"

#include <QColor>
#include <QFile>

#include <eng/buffer.hpp>
#include <eng/base64.hpp>
#include <eng/crc.hpp>

ProgramModel::ProgramModel()
    : QAbstractTableModel()
{
    char const *LIAEM_RW_PATH = std::getenv("LIAEM_RW_PATH");

    path_ = LIAEM_RW_PATH ? LIAEM_RW_PATH : "./home-root";
    path_ /= "programs";

    reset();
}

void ProgramModel::change_sprayer(std::size_t rid, std::size_t id) noexcept
{
    auto &sprayer = program_.main_ops[rid].sprayer;
    sprayer[id] = !sprayer[id];
    dataChanged(createIndex(edited_row_, 0), createIndex(edited_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_main(std::size_t ctype, std::size_t rid, std::size_t id, double value) noexcept
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

    // Приходят в об/мин, программа хранит в град/сек
    case ProgramModelHeader::Spin:
        op.spin[id] = value * 6;
        break;

    // Приходят в мм или градусах, программа хранит в мм или градусах
    case ProgramModelHeader::TargetPos:
        op.target.pos[id] = value;
        break;

    // Приходят в мм/сек или об/мин, программа хранит в мм/cек или град/сек
    case ProgramModelHeader::TargetSpeed:
        op.target.speed = (id == 0) ? value : value * 6;
        break;

    default:
        return;
    }

    dataChanged(createIndex(edited_row_, 0), createIndex(edited_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_pause(std::size_t rid, std::uint64_t v) noexcept
{
    auto &op = program_.pause_ops[rid];
    op.msec = v;
    dataChanged(createIndex(edited_row_, 0), createIndex(edited_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_loop(std::size_t rid, std::size_t opid, std::size_t N) noexcept
{
    auto &op = program_.loop_ops[rid];
    op.opid = opid;
    op.N = N;
    dataChanged(createIndex(edited_row_, 0), createIndex(edited_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_fc(std::size_t rid, float p, float i, float sec) noexcept
{
    auto &op = program_.fc_ops[rid];
    op.p = p * 1000;
    op.i = i;
    op.tv = sec;
    dataChanged(createIndex(edited_row_, 0), createIndex(edited_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::change_center(std::size_t rid, centering_type type, float shift) noexcept
{
    auto &op = program_.center_ops[rid];
    op.type = type;
    op.shift = shift;
    dataChanged(createIndex(edited_row_, 0), createIndex(edited_row_, ProgramModelHeader::column_count(program_) - 1));
}

void ProgramModel::clear_current_row()
{
    if (!current_row_.has_value())
        return;

    std::size_t cc = ProgramModelHeader::column_count(program_);

    dataChanged(createIndex(*current_row_, 0), createIndex(*current_row_, cc - 1));
    current_row_.reset();
}

void ProgramModel::set_current_row(std::size_t row) noexcept
{
    std::size_t cc = ProgramModelHeader::column_count(program_);

    if (current_row_ && *current_row_ == row)
    {
        dataChanged(createIndex(*current_row_, 0), createIndex(*current_row_, cc - 1));
        current_row_.reset();
        return;
    }

    dataChanged(createIndex(row, 0), createIndex(row, cc - 1));
    current_row_ = row;
}

void ProgramModel::set_current_phase(std::size_t row) noexcept
{
    std::size_t cc = ProgramModelHeader::column_count(program_);
    dataChanged(createIndex(row, 0), createIndex(row, cc - 1));
    current_row_ = row;
}

std::size_t ProgramModel::last_insert_row() const noexcept
{
    if (current_row_.has_value())
        return *current_row_;
    else
        return program_.phases.size() - 1;
}

// Добавляем новую операцию, если это перавая то все по дефолту,
// если уже есть операции то копия последней
void ProgramModel::add_op(program::op_type type) noexcept
{
    std::size_t irow = !current_row_.has_value() ? program_.phases.size() : *current_row_;
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
}

void ProgramModel::add_main_op(bool absolute) noexcept
{
    std::size_t irow = !current_row_.has_value() ? program_.phases.size() : *current_row_;
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
    if (!current_row_.has_value() || *current_row_ >= program_.rows())
        return;

    auto [type, rid] = program_.op_info(*current_row_);

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
    plist.erase(plist.begin() + *current_row_);

    beginRemoveRows(QModelIndex(), *current_row_, *current_row_);
    endRemoveRows();

    if (*current_row_ >= program_.rows())
    {
        if (*current_row_ == 0)
        {
            current_row_.reset();
            return;
        }
        else
        {
            *current_row_ -= 1;
        }

        std::size_t cc = ProgramModelHeader::column_count(program_);
        dataChanged(createIndex(*current_row_, 0), createIndex(*current_row_, cc - 1));
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
#ifdef BUILDROOT
            return QString::number(row + 1);
#else
            return QString::number(row);
#endif
        break;

    case Qt::BackgroundRole:
        if (col == 0)
            return (current_row_ && (row == *current_row_)) ? QColor("#ffb800") : QVariant();
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
                std::snprintf(buf, sizeof(buf), "Переход на [ %u ] N=%u", op.opid + 1, op.N);
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
        return (op.target.speed == prev.target.speed);
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
        return QString::number(op.spin[id] / 6, 'f', 2);
    case ProgramModelHeader::TargetPos:
        return QString(op.absolute ? "" : "Δ ") + QString::number(op.target.pos[id], 'f', 2);
    case ProgramModelHeader::TargetSpeed:
        if (id == 0) return QString::number(op.target.speed, 'f', 2);
        return QString::number(op.target.speed / 6, 'f', 2);
    }

    return "";
}

std::string ProgramModel::get_base64_program() const noexcept
{
    eng::buffer::id_t buf = eng::buffer::create();
    program_.save(buf);
    std::string result{ eng::base64::encode(eng::buffer::get_content_region(buf)) };
    eng::buffer::destroy(buf);
    return result;
}

void ProgramModel::reset() noexcept
{
    name_ = "";
    comments_ = "";
    current_row_.reset();

    if (program_.rows())
    {
        beginRemoveRows(QModelIndex(), 0, program_.rows() - 1);
        endRemoveRows();
    }

    program_.clear();

    program_.fc_count = 1;
    program_.sprayer_count = 3;
    program_.s_axis = { 'V' };
    program_.t_axis = { 'X', 'Y', 'Z', 'V' };
}

bool ProgramModel::empty() const noexcept
{
    return program_.rows() == 0;
}

bool ProgramModel::save_to_file() const noexcept
{
    if (!std::filesystem::exists(path_))
        std::filesystem::create_directories(path_);

    if (name_.isEmpty())
        return false;

    QString fname((path_ / name_.toUtf8().constData()).c_str());

    eng::buffer::id_t buf = eng::buffer::create();

    std::string comm(comments_.toUtf8().constData());
    eng::buffer::append<std::uint32_t>(buf, comm.length());
    eng::buffer::append(buf, comm);

    auto span = eng::buffer::get_content_region(buf);

    program_.save(buf);

    span = eng::buffer::get_content_region(buf);

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        eng::buffer::destroy(buf);
        return false;
    }

    span = eng::buffer::get_content_region(buf);
    std::uint32_t crc = eng::crc::crc32(span.begin(), span.end());
    int ret = file.write(reinterpret_cast<char const*>(&crc), sizeof(crc));
    if (ret != sizeof(crc))
    {
        eng::buffer::destroy(buf);
        return false;
    }

    auto content_size = eng::buffer::content_size(buf);
    ret = file.write(eng::buffer::content_c_str(buf), content_size);
    if (ret != content_size)
    {
        eng::buffer::destroy(buf);
        return false;
    }

    eng::buffer::destroy(buf);
    return true;
}

bool ProgramModel::load_program_comments(QString fname, QString &comments) noexcept
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data(file.readAll());
    if (data.size() < sizeof(std::uint32_t))
        return false;

    eng::buffer::id_t buf = eng::buffer::create();
    eng::buffer::append(buf, data.data(), data.size());

    auto span = eng::buffer::get_content_region(buf);
    std::uint32_t crc = eng::read_cast<std::uint32_t>(span);
    std::uint32_t fcrc = eng::crc::crc32(span.begin(), span.end());

    if (crc != fcrc)
    {
        eng::buffer::destroy(buf);
        return false;
    }

    std::uint32_t slen = eng::read_cast<std::uint32_t>(span);
    comments = QString(QByteArray(reinterpret_cast<char const *>(span.data()), slen));

    eng::buffer::destroy(buf);

    return true;
}

bool ProgramModel::load_from_file(QString const& fname) noexcept
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data(file.readAll());
    if (data.size() < sizeof(std::uint32_t))
        return false;

    eng::buffer::id_t buf = eng::buffer::create();
    eng::buffer::append(buf, data.data(), data.size());

    auto span = eng::buffer::get_content_region(buf);
    std::uint32_t crc = eng::read_cast<std::uint32_t>(span);
    std::uint32_t fcrc = eng::crc::crc32(span.begin(), span.end());

    if (crc != fcrc)
    {
        eng::buffer::destroy(buf);
        return false;
    }

    reset();

    std::uint32_t slen = eng::read_cast<std::uint32_t>(span);
    comments_ = QString(QByteArray(reinterpret_cast<char const *>(span.data()), slen));
    span = span.subspan(slen);

    program tp;
    tp.load(span);

    if (tp.rows())
    {
        beginInsertRows(QModelIndex(), 0, tp.rows() - 1);
        program_ = std::move(tp);
        endInsertRows();
    }

    eng::buffer::destroy(buf);

    return true;
}

bool ProgramModel::load_from_usb_file(QString const& fname) noexcept
{
    return false;
}

bool ProgramModel::load_from_local_file(QString name) noexcept
{
    if (!load_from_file((path_ / name.toUtf8().constData()).c_str()))
        return false;

    name_ = name;

    return true;
}

