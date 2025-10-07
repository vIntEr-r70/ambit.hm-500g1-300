#include "AutoEditWidget.h"
#include "ProgramModel.h"
#include "AutoParamKeyboard.h"
#include "EditorMessageBox.h"
#include "ProgramModelHeader.h"

#include <Widgets/ValueSetString.h>
#include <Widgets/IconButton.h>

#include <QVBoxLayout>
#include <QTableView>

#include <aem/log.h>

AutoEditWidget::AutoEditWidget(QWidget *parent, ProgramModel &model) noexcept
    : QWidget(parent)
    , model_(model)
{
    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        QHBoxLayout *hL = new QHBoxLayout();
        {
            name_ = new ValueSetString(this);
            connect(name_, &ValueSetString::onValueChanged, [this] 
            { 
                model_.set_name(name_->value());
                need_save(true); 
            });
            name_->setTitle("Название программы");
            name_->setMaximumWidth(400);
            name_->setMinimumWidth(400);
            hL->addWidget(name_);

            hL->addSpacing(50);

            IconButton *btn_exit_ = new IconButton(this, ":/check-mark");
            connect(btn_exit_, &IconButton::clicked, [this] { do_exit(); });
            btn_exit_->setBgColor("#8a8a8a");
            hL->addWidget(btn_exit_);

            btn_save_ = new IconButton(this, ":/SaveFile");
            connect(btn_save_, &IconButton::clicked, [this] { do_save();});
            btn_save_->setBgColor("#8a8a8a");
            hL->addWidget(btn_save_);

            hL->addStretch();

            QList<QPair<QString, TableAc>> tableAcBtns = {
                { ":/editor.main-op",       AddMainOp     },
                { ":/editor.main-rel-op",   AddRelMainOp  },
                { ":/editor.pause",         AddPauseOp    },
                { ":/editor.inductor",      AddTimedFCOp  },
                { ":/editor.center",        AddCenterOp   },
                { ":/AddGoTo",              AddGoToOp     },
                { ":/PhaseRemove",          DeleteOp      }
            };

            for (auto const& ac : tableAcBtns)
            {
                IconButton *btn = new IconButton(this, ac.first, ac.second);
                btn->setBgColor("#8a8a8a");
                connect(btn, &IconButton::clickedId, [this](int op) { make_table_op(static_cast<TableAc>(op)); });
                hL->addWidget(btn);
            }
        }
        vL->addLayout(hL);

        comments_ = new ValueSetString(this);
        connect(comments_, &ValueSetString::onValueChanged, [this] 
        { 
            model_.set_comments(comments_->value());
            need_save(true); 
        });
        comments_->setTitle("Коментарии");
        vL->addWidget(comments_);
    }

    kb_ = new AutoParamKeyboard(this);
    msg_ = new EditorMessageBox(this);
}

void AutoEditWidget::init()
{
    name_->set_value(model_.name());
    comments_->set_value(model_.comments());

    need_save(false);
}

void AutoEditWidget::set_position(char axis, float pos) 
{
    real_pos_[axis] = pos;
}

void AutoEditWidget::make_table_op(TableAc ac) noexcept
{
    need_save(true);

    switch(ac)
    {
    case AddMainOp:
        model_.add_main_op(true);
        return;
    case AddRelMainOp:
        model_.add_main_op(false);
        return;
    case DeleteOp:
        model_.remove_op();
        return;
    case AddPauseOp:
        model_.add_op(program::op_type::pause);
        break;
    case AddGoToOp:
        model_.add_op(program::op_type::loop);
        break;
    case AddTimedFCOp:
        model_.add_op(program::op_type::fc);
        break;
    case AddCenterOp:
        model_.add_op(program::op_type::center);
        break;
    }
}

void AutoEditWidget::tableCellSelect(QModelIndex index) noexcept
{
    // aem::log::info("AutoEditWidget::tableCellSelect: row = {}, col = {}", index.row(), index.column());

    std::size_t row = index.row();
    std::size_t col = index.column();

    auto [ctype, id] = ProgramModelHeader::cell(model_.prog(), col);
    if (ctype == ProgramModelHeader::Num)
    {
        model_.set_current_row(row);
        return;
    }

    auto [type, rid] = model_.prog().op_info(row);

    if (type == program::op_type::main)
    {
        if (ctype == ProgramModelHeader::Sprayer)
        {
            model_.change_sprayer(rid, id);
            need_save(true);
            return;
        }

        float min = 0;
        float max = 0;

        QString v = index.data().toString();
        QString title = ProgramModelHeader::title(model_.prog(), ctype, id);

        // Если это позиционная координата
        float pos = NAN;
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
            kb_->show_main(title, v, pos, min, max, [this, ctype, rid, id] (float v)
            {
                model_.change_main(ctype, rid, id, v);
                need_save(true);
            });
        }
        else
        {
            // Иначе показываем обычный диалог
            kb_->show_main(title, v, min, max, [this, ctype, rid, id] (float v)
            {
                model_.change_main(ctype, rid, id, v);
                need_save(true);
            });
        }
    }
    else
    {
        switch(type)
        {
        case program::op_type::pause: {
            auto &op = model_.prog().pause_ops[rid];
            kb_->show_pause(op.msec, [this, rid] (aem::uint64 v)
            {
                model_.change_pause(rid, v);
                need_save(true);
            });
            break; }

        case program::op_type::loop: {
            auto &op = model_.prog().loop_ops[rid];
            kb_->show_loop(op.opid, op.N, [this, rid] (std::size_t opid, std::size_t N)
            {
                model_.change_loop(rid, opid, N);
                need_save(true);
            });
            break; }

        case program::op_type::fc: {
            auto &op = model_.prog().fc_ops[rid];
            kb_->show_fc(op.p, op.i, op.tv, [this, rid] (float p, float i, float sec)
            {
                model_.change_fc(rid, p, i, sec);
                need_save(true);
            });
            break; }

        case program::op_type::center: {
            auto &op = model_.prog().center_ops[rid];
            kb_->show_center(op.type, op.shift, [this, rid] (centering_type type, float shift)
            {
                model_.change_center(rid, type, shift);
                need_save(true);
            });
            break; }
        }
    }
}

void AutoEditWidget::edit_main_op(std::size_t id, std::size_t column)
{
    // model_.edit_main_op(id, column, [this](auto ptype, auto pid, nlohmann::json const&)
    // {
    //     
    //
    // });

    // auto [ptype, pid] = model_.get_op_info(column);
    // switch(ptype)
    // {
    // case ProgramModel::param_type_t::as_float:
    //     break;
    // case ProgramModel::param_type_t::as_bool:
    //
    //     break;
    // }
}

void AutoEditWidget::edit_pause_op(std::size_t)
{
}
void AutoEditWidget::edit_fc_op(std::size_t)
{
}
void AutoEditWidget::edit_loop_op(std::size_t)
{
}

void AutoEditWidget::need_save(bool dirty) noexcept
{
    btn_save_->setVisible(dirty);
}

void AutoEditWidget::do_exit() noexcept
{
    if (btn_save_->isVisible())
    {
        msg_->show_question("Продолжить без сохранения изменений?", [this]
        {
            emit make_done();
        });
    }
    else
    {
        emit make_done();
    }
}

void AutoEditWidget::do_save() noexcept
{
    // TODO: touch
    if (model_.name().isEmpty())
    {
        name_->external_touch();
        return;
    }

    if (!model_.save_to_file())
    {
        msg_->show_error("Не удалось сохранить программу!");
        return;
    }

    need_save(false);
}

void AutoEditWidget::tableCellClicked(QModelIndex index) noexcept
{
    // aem::log::info("AutoEditWidget::tableCellClicked: row = {}, col = {}", index.row(), index.column());
    // model_.set_current_row(index.row() < 0 ? aem::MaxSizeT : index.row());

    // if (!editor_->allowEdit())
    //     return;
    // std::size_t row = index.row();
    // std::size_t col = index.column();
    // if (col == 0)
    //     return;
    // tblCell_ = std::make_pair(row, col);
    // Phase* phase = program_->phase(row);
    // Op const* op = dynamic_cast<Op const*>(phase);
    // if (op == nullptr)
    // {
    //     if (dynamic_cast<GoTo const*>(phase))
    //         kbDlg_->setTitle("Параметры цикла\n[номер этапа] [кол-во повторов]");
    //     else if (dynamic_cast<Pause const*>(phase))
    //         kbDlg_->setTitle("Время ожидания\n[часов] [минут] [секунд]");
    //     else if (dynamic_cast<TimedFC const*>(phase))
    //         kbDlg_->setTitle("Прецизионный нагрев\n[Мощность, кВт] [Ток, А] [Время, секунд]");
    //     else
    //         kbDlg_->setTitle("");
    //     kbDlg_->show(phase);
    //     return;
    // }
    // OpItem* opItem = program_->getOpItem(row, col - 1);
    // assert(opItem != nullptr);
    // {
    //     OpBool* op = dynamic_cast<OpBool*>(opItem);
    //     if (op)
    //     {
    //         op->invert();
    //         tableModelProgram_.updateCell(row, col);
    //         editor_->setModifyFlag();
    //         return;
    //     }
    // }
    // {
    //     OpFloat* op = dynamic_cast<OpFloat*>(opItem);
    //     if (op)
    //     {
    //         //! Если задаем уставку скорости, необходимо ограничить её
    //         if (dynamic_cast<OpSpeed*>(op))
    //         {
    //             kbDlg_->setValueRange(0, 0, UnitsCalc::toSpeed(false, maxAxisAllowSpeed_));
    //             kbDlg_->setValueRange(1, 0, UnitsCalc::toSpeed(true, maxAxisAllowSpeed_));
    //         }
    //         else
    //         {
    //             kbDlg_->setValueRange(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
    //         }
    //         kbDlg_->setTitle(QString::fromStdString(programOpFactory_.itemName(col - 1)));
    //         kbDlg_->show(op);
    //         return;
    //     }
    // }
}
