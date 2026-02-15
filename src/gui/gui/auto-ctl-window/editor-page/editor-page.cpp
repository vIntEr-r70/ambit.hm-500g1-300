#include "editor-page.hpp"
#include "program-model-editor.hpp"
#include "../common/program-widget.hpp"
#include "../common/program-record.hpp"
#include "../common/ProgramModelHeader.h"
#include "AutoParamKeyboard.h"
#include "common/load-axis-list.hpp"
#include "EditorMessageBox.h"

#include <QVBoxLayout>
#include <QTableView>
#include <QFile>

#include <eng/log.hpp>
#include <eng/crc.hpp>

editor_page::editor_page(QWidget *parent)
    : QWidget(parent)
    , eng::sibus::node("program-editor")
{
    auto LIAEM_RW_PATH = std::getenv("LIAEM_RW_PATH");
    path_ = LIAEM_RW_PATH ? LIAEM_RW_PATH : ".";
    path_ /= "programs";

    model_ = new program_model_editor();

    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        header_ = new editor_page_header_widget(this);
        connect(header_, &editor_page_header_widget::do_exit, [this] { make_exit(); });
        connect(header_, &editor_page_header_widget::do_play, [this] { make_play(); });
        connect(header_, &editor_page_header_widget::do_save, [this] { make_save(); });
        connect(header_, &editor_page_header_widget::make_table_op,
            [this](editor_page_header_widget::TableAc ac) { make_table_op(ac); });
        vL->addWidget(header_);

        vL->addSpacing(20);

        program_widget_ = new program_widget(this, *model_);
        connect(program_widget_->tbody(), &QTableView::pressed, [this](QModelIndex index) {
            table_cell_select(index);
        });
        vL->addWidget(program_widget_);
    }

    ambit::load_axis_list([this](char axis, std::string_view, bool)
    {
        node::add_input_port(std::string(1, axis), [this, axis](eng::abc::pack args)
        {
            positions_[axis] = eng::abc::get<double>(args);
        });
    });

    empty_program_.fc_count = 1;
    empty_program_.sprayer_count = 3;
    empty_program_.s_axis = { 'V' };
    empty_program_.t_axis = { 'X', 'Y', 'Z', 'V' };

    kb_ = new AutoParamKeyboard(this);

    msg_ = new EditorMessageBox(this);
}

void editor_page::init(program_record_t const *r)
{
    if (r == nullptr)
    {
        fname_.clear();

        model_->set_program(empty_program_);
        header_->set_program_info("", "");
    }
    else
    {
        fname_ = r->filename;

        auto const &comm = r->comments;

        header_->set_program_info(
            QString::fromUtf8(fname_.data(), fname_.length()),
            QString::fromUtf8(comm.data(), comm.length()));

        // Данные самой программы
        auto span = std::span<std::byte const>{
                r->data.data() + r->head_size,
                r->data.size() - r->head_size
            };

        {
            program prog;
            prog.load(span);
            model_->set_program(std::move(prog));
        }
    }

    header_->need_save(false);
}

void editor_page::make_table_op(editor_page_header_widget::TableAc ac)
{
    header_->need_save(true);

    using TableAc = editor_page_header_widget::TableAc;

    std::size_t irow = model_->current_row();

    switch(ac)
    {
    case TableAc::AddMainOp:
        model_->add_main_op(true);
        program_widget_->update_view();
        return;
    case TableAc::AddRelMainOp:
        model_->add_main_op(false);
        program_widget_->update_view();
        return;
    case TableAc::DeleteOp:
        model_->remove_op();
        program_widget_->update_view();
        return;
    case TableAc::AddPauseOp:
        model_->add_op(program::op_type::pause);
        break;
    case TableAc::AddGoToOp:
        model_->add_op(program::op_type::loop);
        break;
    case TableAc::AddTimedFCOp:
        model_->add_op(program::op_type::fc);
        break;
    case TableAc::AddCenterOp:
        model_->add_op(program::op_type::center);
        break;
    }

    program_widget_->update_table_row_span(irow);
}

void editor_page::table_cell_select(QModelIndex index)
{
    std::size_t row = index.row();
    std::size_t col = index.column();

    auto [ctype, id] = ProgramModelHeader::cell(model_->prog(), col);
    if (ctype == ProgramModelHeader::Num)
    {
        model_->set_current_row(row);
        return;
    }

    auto [type, rid] = model_->prog().op_info(row);

    switch(type)
    {
    case program::op_type::main: {
        if (ctype == ProgramModelHeader::Sprayer)
        {
            model_->change_sprayer(row, rid, id);
            header_->need_save(true);
            return;
        }

        double min = 0;
        double max = 0;

        QString v = index.data().toString();
        if (ctype == ProgramModelHeader::TargetPos)
        {
            auto vl = v.split(' ');
            if (!vl.empty()) v = vl.back();
        }

        QString title = ProgramModelHeader::title(model_->prog(), ctype, id);

        // Если это позиционная координата
        double pos = NAN;
        if (ctype == ProgramModelHeader::TargetPos)
        {
            // Пробуем получить текущую позицию
            auto it = positions_.find(model_->prog().t_axis[id]);
            if (it != positions_.end())
                pos = it->second;
        }

        if (!std::isnan(pos))
        {
            // Если текущая позиция есть, показываем диалог с возможностью ее использовать
            kb_->show_main(title, v, pos, min, max, [this, row, ctype, rid, id] (double v)
            {
                model_->change_main(row, ctype, rid, id, v);
                header_->need_save(true);
            });
        }
        else
        {
            // Иначе показываем обычный диалог
            kb_->show_main(title, v, min, max, [this, row, ctype, rid, id] (double v)
            {
                model_->change_main(row, ctype, rid, id, v);
                header_->need_save(true);
            });
        }
        break; }
    case program::op_type::pause: {
        auto &op = model_->prog().pause_ops[rid];
        kb_->show_pause(op.msec, [this, row, rid] (std::uint64_t v)
        {
            model_->change_pause(row, rid, v);
            header_->need_save(true);
        });
        break; }

    case program::op_type::loop: {
        auto &op = model_->prog().loop_ops[rid];
        kb_->show_loop(op.opid, op.N, [this, row, rid] (std::size_t opid, std::size_t N)
        {
            model_->change_loop(row, rid, opid, N);
            header_->need_save(true);
        });
        break; }

    case program::op_type::fc: {
        auto &op = model_->prog().fc_ops[rid];
        kb_->show_fc(op.p, op.i, op.tv, [this, row, rid] (double p, double i, double sec)
        {
            model_->change_fc(row, rid, p, i, sec);
            header_->need_save(true);
        });
        break; }

    case program::op_type::center: {
        auto &op = model_->prog().center_ops[rid];
        kb_->show_center(op.type, op.shift, [this, row, rid] (centering_type type, double shift)
        {
            model_->change_center(row, rid, type, shift);
            header_->need_save(true);
        });
        break; }
    }
}

void editor_page::make_save()
{
    if (header_->name().isEmpty())
    {
        msg_->show_error("Не задано имя программы");
        return;
    }

    std::string fname{ header_->name().toUtf8().constData() };
    std::filesystem::path path{ path_ / fname };

    if (fname_ != fname)
    {
        if (std::filesystem::exists(path))
        {
            msg_->show_error("Программа с таким именем уже есть");
            return;
        }
        fname_ = fname;
    }

    eng::buffer::id_t buf = eng::buffer::create();

    std::string comments(header_->comments().toUtf8().constData());
    eng::buffer::append<std::uint32_t>(buf, comments.length());
    eng::buffer::append(buf, comments);

    auto span = eng::buffer::get_content_region(buf);

    model_->prog().save(buf);

    span = eng::buffer::get_content_region(buf);

    QFile file(path.c_str());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        eng::buffer::destroy(buf);
        msg_->show_error("Не удалось сохранить программу");
        return;
    }

    span = eng::buffer::get_content_region(buf);
    std::uint32_t crc = eng::crc::crc32(span.begin(), span.end());
    int ret = file.write(reinterpret_cast<char const*>(&crc), sizeof(crc));
    if (ret != sizeof(crc))
    {
        eng::buffer::destroy(buf);
        msg_->show_error("Не удалось сохранить программу");
        return;
    }

    auto content_size = eng::buffer::content_size(buf);
    ret = file.write(eng::buffer::content_c_str(buf), content_size);
    if (ret != content_size)
    {
        eng::buffer::destroy(buf);
        msg_->show_error("Не удалось сохранить программу");
        return;
    }

    eng::buffer::destroy(buf);

    header_->need_save(false);
}

void editor_page::make_play()
{
}

void editor_page::make_exit()
{
    if (header_->need_save())
    {
        msg_->show_question("Выйти без сохранения изменений?", [this]
        {
            emit make_done();
        });
    }
    else
    {
        emit make_done();
    }
}

