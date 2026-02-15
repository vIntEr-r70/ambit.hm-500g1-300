#include "program-model-editor.hpp"
#include "../common/ProgramModelHeader.h"

#include <QColor>
#include <QFile>

#include <eng/buffer.hpp>
#include <eng/base64.hpp>
#include <eng/crc.hpp>

void program_model_editor::set_current_row(std::size_t row)
{
    if (current_row_ == row)
    {
        current_row_ = program_.rows();
        dataChanged(createIndex(row, 0), createIndex(row, 0));
        return;
    }

    if (current_row_ != program_.rows())
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, 0));

    current_row_ = row;
    dataChanged(createIndex(row, 0), createIndex(row, 0));
}

void program_model_editor::change_sprayer(std::size_t row, std::size_t rid, std::size_t id)
{
    auto &sprayer = program_.main_ops[rid].sprayer;
    sprayer[id] = !sprayer[id];
    dataChanged(createIndex(row, 0), createIndex(row, ProgramModelHeader::column_count(program_) - 1));
}

void program_model_editor::change_main(std::size_t row, std::size_t ctype, std::size_t rid, std::size_t id, double value)
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

    dataChanged(createIndex(row, 0), createIndex(row, ProgramModelHeader::column_count(program_) - 1));
}

void program_model_editor::change_pause(std::size_t row, std::size_t rid, std::uint64_t v)
{
    auto &op = program_.pause_ops[rid];
    op.msec = v;
    dataChanged(createIndex(row, 0), createIndex(row, ProgramModelHeader::column_count(program_) - 1));
}

void program_model_editor::change_loop(std::size_t row, std::size_t rid, std::size_t opid, std::size_t N)
{
    auto &op = program_.loop_ops[rid];
    op.opid = opid;
    op.N = N;
    dataChanged(createIndex(row, 0), createIndex(row, ProgramModelHeader::column_count(program_) - 1));
}

void program_model_editor::change_fc(std::size_t row, std::size_t rid, float p, float i, float sec)
{
    auto &op = program_.fc_ops[rid];
    op.p = p * 1000;
    op.i = i;
    op.tv = sec;
    dataChanged(createIndex(row, 0), createIndex(row, ProgramModelHeader::column_count(program_) - 1));
}

void program_model_editor::change_center(std::size_t row, std::size_t rid, centering_type type, float shift)
{
    auto &op = program_.center_ops[rid];
    op.type = type;
    op.shift = shift;
    dataChanged(createIndex(row, 0), createIndex(row, ProgramModelHeader::column_count(program_) - 1));
}

// void program_model_editor::clear_current_row()
// {
//     if (!current_row_.has_value())
//         return;
//
//     std::size_t cc = ProgramModelHeader::column_count(program_);
//
//     dataChanged(createIndex(*current_row_, 0), createIndex(*current_row_, cc - 1));
//     current_row_.reset();
// }

// void program_model_editor::set_current_phase(std::size_t row) noexcept
// {
//     std::size_t cc = ProgramModelHeader::column_count(program_);
//     dataChanged(createIndex(row, 0), createIndex(row, cc - 1));
//     current_row_ = row;
// }

// std::size_t program_model_editor::last_insert_row() const noexcept
// {
//     if (current_row_.has_value())
//         return *current_row_;
//     else
//         return program_.phases.size() - 1;
// }

// Добавляем новую операцию, если это перавая то все по дефолту,
// если уже есть операции то копия последней
void program_model_editor::add_op(program::op_type type) noexcept
{
    std::size_t irow = current_row_;
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
    current_row_ += 1;
    endInsertRows();
}

void program_model_editor::add_main_op(bool absolute) noexcept
{
    std::size_t irow = current_row_;
    auto rid = program_.get_rid(program::op_type::main, irow);
    auto &oplist = program_.main_ops;

    if (oplist.empty())
        program_.add_default_main_op();
    else
        oplist.insert(oplist.begin() + rid, oplist[(rid == oplist.size()) ? (rid - 1) : rid]);

    oplist[rid].absolute = absolute;

    beginInsertRows(QModelIndex(), irow, irow);
    program_.phases.insert(program_.phases.begin() + irow, program::op_type::main);
    current_row_ += 1;
    endInsertRows();
}

void program_model_editor::remove_op() noexcept
{
    if (current_row_ == program_.rows())
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

    beginRemoveRows(QModelIndex(), current_row_, current_row_);
    auto &plist = program_.phases;
    plist.erase(plist.begin() + current_row_);
    endRemoveRows();

    if (current_row_ >= program_.rows())
    {
        if (current_row_ == 0)
            return;
        current_row_ -= 1;

        std::size_t cc = ProgramModelHeader::column_count(program_);
        dataChanged(createIndex(current_row_, 0), createIndex(current_row_, cc - 1));
    }
}

// bool ProgramModel::save_to_file() const noexcept
// {
//     if (!std::filesystem::exists(path_))
//         std::filesystem::create_directories(path_);
//
//     if (name_.isEmpty())
//         return false;
//
//     QString fname((path_ / name_.toUtf8().constData()).c_str());
//
//     eng::buffer::id_t buf = eng::buffer::create();
//
//     std::string comm(comments_.toUtf8().constData());
//     eng::buffer::append<std::uint32_t>(buf, comm.length());
//     eng::buffer::append(buf, comm);
//
//     auto span = eng::buffer::get_content_region(buf);
//
//     program_.save(buf);
//
//     span = eng::buffer::get_content_region(buf);
//
//     QFile file(fname);
//     if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
//     {
//         eng::buffer::destroy(buf);
//         return false;
//     }
//
//     span = eng::buffer::get_content_region(buf);
//     std::uint32_t crc = eng::crc::crc32(span.begin(), span.end());
//     int ret = file.write(reinterpret_cast<char const*>(&crc), sizeof(crc));
//     if (ret != sizeof(crc))
//     {
//         eng::buffer::destroy(buf);
//         return false;
//     }
//
//     auto content_size = eng::buffer::content_size(buf);
//     ret = file.write(eng::buffer::content_c_str(buf), content_size);
//     if (ret != content_size)
//     {
//         eng::buffer::destroy(buf);
//         return false;
//     }
//
//     eng::buffer::destroy(buf);
//     return true;
// }

// bool ProgramModel::load_program_comments(QString fname, QString &comments) noexcept
// {
//     QFile file(fname);
//     if (!file.open(QIODevice::ReadOnly))
//         return false;
//
//     QByteArray data(file.readAll());
//     if (data.size() < sizeof(std::uint32_t))
//         return false;
//
//     eng::buffer::id_t buf = eng::buffer::create();
//     eng::buffer::append(buf, data.data(), data.size());
//
//     auto span = eng::buffer::get_content_region(buf);
//     std::uint32_t crc = eng::read_cast<std::uint32_t>(span);
//     std::uint32_t fcrc = eng::crc::crc32(span.begin(), span.end());
//
//     if (crc != fcrc)
//     {
//         eng::buffer::destroy(buf);
//         return false;
//     }
//
//     std::uint32_t slen = eng::read_cast<std::uint32_t>(span);
//     comments = QString(QByteArray(reinterpret_cast<char const *>(span.data()), slen));
//
//     eng::buffer::destroy(buf);
//
//     return true;
// }

// bool ProgramModel::load_from_file(QString const& fname) noexcept
// {
//     QFile file(fname);
//     if (!file.open(QIODevice::ReadOnly))
//         return false;
//
//     QByteArray data(file.readAll());
//     if (data.size() < sizeof(std::uint32_t))
//         return false;
//
//     eng::buffer::id_t buf = eng::buffer::create();
//     eng::buffer::append(buf, data.data(), data.size());
//
//     auto span = eng::buffer::get_content_region(buf);
//     std::uint32_t crc = eng::read_cast<std::uint32_t>(span);
//     std::uint32_t fcrc = eng::crc::crc32(span.begin(), span.end());
//
//     if (crc != fcrc)
//     {
//         eng::buffer::destroy(buf);
//         return false;
//     }
//
//     reset();
//
//     std::uint32_t slen = eng::read_cast<std::uint32_t>(span);
//     comments_ = QString(QByteArray(reinterpret_cast<char const *>(span.data()), slen));
//     span = span.subspan(slen);
//
//     program tp;
//     tp.load(span);
//
//     if (tp.rows())
//     {
//         beginInsertRows(QModelIndex(), 0, tp.rows() - 1);
//         program_ = std::move(tp);
//         endInsertRows();
//     }
//
//     eng::buffer::destroy(buf);
//
//     return true;
// }

//
// bool ProgramModel::load_from_local_file(QString name) noexcept
// {
//     if (!load_from_file((path_ / name.toUtf8().constData()).c_str()))
//         return false;
//
//     name_ = name;
//
//     return true;
// }
//
