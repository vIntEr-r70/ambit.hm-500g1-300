#pragma once

#include <QWidget>

#include <aem/types.h>

#include <array>

class QLabel;
class ValueViewReal;
class QFrame;

class AxisCtlItemWidget
    : public QWidget
{
    Q_OBJECT

public:

    enum
    {
        NotActive,
        Active,
        SelectAsCtrl,

        Count
    };

public:

    AxisCtlItemWidget(QWidget*, char);

signals:

    void onMoveToClick(char);

public:

    void update_name(std::string_view);

public:

    void apply_self_axis() noexcept;

    void nf_speed(float) noexcept;

    void nf_ls_min(bool v) noexcept { ls_min_max(0, v); }

    void nf_ls_max(bool v) noexcept { ls_min_max(1, v); }

    void nf_pos(float) noexcept;

public:

    void set_sys_ctrl_mode_axis(char) noexcept;

    void set_sys_mode(unsigned char) noexcept;

    void set_sys_ctrl(unsigned char) noexcept;

    void set_sys_error(unsigned int) noexcept;

private:

    void mousePressEvent(QMouseEvent*) override final;

private:

    void ls_min_max(std::size_t, bool) noexcept;

    void setStatus(uint8_t);

    void updateGui();

private:

    char axis_;

    QString dlgTitle_;

    QLabel* lblHeader_;
    QFrame* lblLS_[2];

    ValueViewReal *vvr_speed_;
    ValueViewReal *vvr_pos_;

    std::array<std::array<QColor, 2>, Count> headClr_;

    float pos_{0.0f};
    float speed_{0.0f};

    aem::uint8 sysMode_;
    aem::uint8 ctlMode_;
    bool ls_[2] = {false, false};

    char ctrlModeAxis_ = 0;
    aem::uint32 error_ = 0;
    aem::uint8 status_ = NotActive;
};

