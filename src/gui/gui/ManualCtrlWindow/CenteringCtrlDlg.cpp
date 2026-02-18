#include "CenteringCtrlDlg.h"
#include <Widgets/RoundButton.h>

#include <QVBoxLayout>
#include <QLabel>

CenteringCtrlDlg::CenteringCtrlDlg(QWidget *parent) noexcept
    : InteractWidget(parent)
{
    setStyleSheet("border-radius: 20px; background-color: #c8c8c8");

    QVBoxLayout* vL = new QVBoxLayout(this);
    vL->setContentsMargins(20, 20, 20, 20);
    {
        lbl_title_ = new QLabel(this);
        lbl_title_->setText("");
        lbl_title_->setStyleSheet("color: white; font: bold 20pt");
        vL->addWidget(lbl_title_);

        vL->addSpacing(20);

        for (std::size_t i = 0; i < 9; ++i)
        {
            QLabel *lbl = new QLabel(this);
            lbl->setStyleSheet("color: white; font: 14pt");
            lbl->setVisible(false); 
            vL->addWidget(lbl);
            lbls_.push_back(lbl);
        }

        vL->addSpacing(20);

        QHBoxLayout *hL = new QHBoxLayout();
        {
            RoundButton *btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { on_start(); });
            btn->setIcon(":/check-mark");
            btn->setBgColor("#29AC39");
            btn->setMinimumWidth(90);
            hL->addWidget(btn);

            hL->addSpacing(50);

            btn = new RoundButton(this);
            connect(btn, &RoundButton::clicked, [this] { on_terminate(); });
            btn->setIcon(":/kbd.dlg.cancel");
            btn->setBgColor("#E55056");
            btn->setMinimumWidth(90);
            hL->addWidget(btn);
        }
        vL->addLayout(hL);
    }
}

void CenteringCtrlDlg::on_start() noexcept
{
    // nlohmann::json::array_t args;
    // if (!type_) // Tooth
    // {
    //     args.push_back("centering");
    //     args.push_back("tooth");
    //     args.push_back({ tooth_shift_, tooth_type_ });
    // }
    // else // Shaft
    // {
    //     args.push_back("centering");
    //     args.push_back("shaft");
    //     args.push_back({});
    // }
    //
    // global::rpc().call("set", std::move(args))
    //     .done([this](nlohmann::json const&)
    //     {
    //         aem::log::info("CenteringCtrlDlg::on_start: OK");
    //         start_timer();
    //     })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("CenteringCtrlDlg::on_start: {}", emsg);
    //     });
}

void CenteringCtrlDlg::on_terminate() noexcept
{
    // global::rpc().call("set", { "centering", "terminate", {} })
    //     .done([this](nlohmann::json const&)
    //     {
    //         if (timer_id_ != -1)
    //             killTimer(timer_id_);
    //         timer_id_ = -1;
    //
    //         InteractWidget::hide(); 
    //     })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("CenteringCtrlDlg::on_terminate: {}", emsg);
    //     });
}

void CenteringCtrlDlg::set_centering_step(int step) noexcept
{
    // aem::log::warn("CenteringCtrlDlg::set_calibrate_step: {}", step);
    //
    // // Центровка успешно завершена, закрываем окно
    // if (step == 0 && !lbl_steps_.empty())
    // {
    //     if (timer_id_ != -1)
    //         killTimer(timer_id_);
    //     timer_id_ = -1;
    //
    //     InteractWidget::hide();
    //
    //     return;
    // }
    //
    // std::size_t ustep = std::labs(step);
    //
    // if (step_ < lbl_steps_.size())
    //     lbl_steps_[step_]->setStyleSheet("color: white; font: 14pt;");
    //
    // step_ = ustep - 1;
    // step_blink_ = false;
    // step_error_ = step < 0;
    //
    // on_step_timer();
}

void init_from_list(auto &&src, auto &&dst, QStringList const& ttls)
{
    std::for_each(dst.begin(), dst.end(), 
            [&](auto lbl) 
            { 
                lbl->setVisible(false); 
            });
    dst.clear();

    std::for_each(ttls.begin(), ttls.end(), 
            [&](auto const& ttl) 
            { 
                dst.push_back(src[dst.size()]);
                dst.back()->setText(ttl);
                dst.back()->setVisible(true); 
            });
}

void CenteringCtrlDlg::show() noexcept
{
    type_ = true;

    lbl_title_->setText("Центровка вала");

    static QStringList const steps { 
        "Поиск первой точки по Y", "Возвращение в исходную точку Y",
        "Поиск второй точки по Y", "Перемещение в центр по Y",
        "Поиск первой точки по X", "Возвращение в исходную точку X",
        "Поиск второй точки по X", "Перемещение в центр по X" };

    init_from_list(lbls_, lbl_steps_, steps);

    InteractWidget::show();
}

void CenteringCtrlDlg::show(float H, bool v) noexcept
{
    type_ = false;
    tooth_shift_ = H;
    tooth_type_ = v;

    lbl_title_->setText(QString("Центровка %1 зуба\nh = %2 мм")
            .arg(v ? "внешнего" : "внутреннего")
            .arg(H, 0, 'f', 2));

    static QStringList const steps { 
        "Поиск первой точки по Y", "Возвращение в исходную точку Y",
        "Поиск второй точки по Y", "Перемещение в центр по Y",
        "Поиск впадины зуба по X", "Откат на h" };

    init_from_list(lbls_, lbl_steps_, steps);

    InteractWidget::show();
}

void CenteringCtrlDlg::start_timer() noexcept
{
    step_error_ = false;
    step_ = 0;
    step_blink_ = false;

    if (timer_id_ == -1)
        timer_id_ = startTimer(500);

    on_step_timer();
}

void CenteringCtrlDlg::on_step_timer() noexcept
{
    if (step_error_)
    {
        if (step_ < lbl_steps_.size())
            lbl_steps_[step_]->setStyleSheet("color: red; font: 14pt;");
        return;
    }

    step_blink_ = !step_blink_;
    
    if (step_ < lbl_steps_.size())
    {
        lbl_steps_[step_]->
            setStyleSheet(QString("color: %1; font: 14pt;").arg(step_blink_ ? "blue" : "white"));
    }
}
