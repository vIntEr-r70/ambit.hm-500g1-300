#include "editor-page.hpp"
#include "editor-page-header-widget.hpp"
#include "program-widget.hpp"
#include "ProgramModel.h"
#include "ProgramModelHeader.h"
#include "AutoParamKeyboard.h"

#include <QVBoxLayout>
#include <QTableView>

#include <eng/log.hpp>

editor_page::editor_page(QWidget *parent, ProgramModel &model)
    : QWidget(parent)
    , model_(model)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        header_ = new editor_page_header_widget(this, model);
        connect(header_, &editor_page_header_widget::make_done, [this] {
            emit make_done();
        });
        connect(header_, &editor_page_header_widget::rows_count_changed, [this](bool all) {
            program_widget_->rows_count_changed(all);
        });
        vL->addWidget(header_);

        program_widget_ = new program_widget(this, model);
        connect(program_widget_->tbody(), &QTableView::pressed, [this](QModelIndex index) {
            table_cell_select(index);
        });
        vL->addWidget(program_widget_);
    }

    kb_ = new AutoParamKeyboard(this);
}

void editor_page::init(QString const &name)
{
    if (name.isEmpty())
        model_.reset();
    else
        model_.load_from_local_file(name);

    program_widget_->rows_count_changed(true);
}

void editor_page::table_cell_select(QModelIndex index)
{
    std::size_t row = index.row();
    std::size_t col = index.column();

    auto [ctype, id] = ProgramModelHeader::cell(model_.prog(), col);
    if (ctype == ProgramModelHeader::Num)
    {
        model_.set_current_row(row);
        return;
    }

    auto [type, rid] = model_.prog().op_info(row);

    model_.set_edited_row(row);
    switch(type)
    {
    case program::op_type::main: {
        if (ctype == ProgramModelHeader::Sprayer)
        {
            model_.change_sprayer(rid, id);
            header_->need_save(true);
            return;
        }

        double min = 0;
        double max = 0;

        QString v = index.data().toString();
        QString title = ProgramModelHeader::title(model_.prog(), ctype, id);

        // Если это позиционная координата
        double pos = NAN;
        if (ctype == ProgramModelHeader::TargetPos)
        {
            // Пробуем получить текущую позицию
            auto it = real_pos_.find(model_.prog().t_axis[id]);
            if (it != real_pos_.end())
                pos = it->second;
        }

        if (!std::isnan(pos))
        {
            // Если текущая позиция есть, показываем диалог с возможностью ее использовать
            kb_->show_main(title, v, pos, min, max, [this, ctype, rid, id] (double v)
            {
                model_.change_main(ctype, rid, id, v);
                header_->need_save(true);
            });
        }
        else
        {
            // Иначе показываем обычный диалог
            kb_->show_main(title, v, min, max, [this, ctype, rid, id] (double v)
            {
                model_.change_main(ctype, rid, id, v);
                header_->need_save(true);
            });
        }
        break; }
    case program::op_type::pause: {
        auto &op = model_.prog().pause_ops[rid];
        kb_->show_pause(op.msec, [this, rid] (std::uint64_t v)
        {
            model_.change_pause(rid, v);
            header_->need_save(true);
        });
        break; }

    case program::op_type::loop: {
        auto &op = model_.prog().loop_ops[rid];
        kb_->show_loop(op.opid, op.N, [this, rid] (std::size_t opid, std::size_t N)
        {
            model_.change_loop(rid, opid, N);
            header_->need_save(true);
        });
        break; }

    case program::op_type::fc: {
        auto &op = model_.prog().fc_ops[rid];
        kb_->show_fc(op.p, op.i, op.tv, [this, rid] (double p, double i, double sec)
        {
            model_.change_fc(rid, p, i, sec);
            header_->need_save(true);
        });
        break; }

    case program::op_type::center: {
        auto &op = model_.prog().center_ops[rid];
        kb_->show_center(op.type, op.shift, [this, rid] (centering_type type, double shift)
        {
            model_.change_center(rid, type, shift);
            header_->need_save(true);
        });
        break; }
    }
}

