#pragma once

#include <InteractWidgets/InteractWidget.h>

#include <eng/sibus/node.hpp>

class RoundButton;

class LockMessageBox
    : public InteractWidget
    , public eng::sibus::node
{
    QWidget *bki_;
    QWidget *emg_;

    eng::sibus::output_wire_id_t bki_reset_;

    RoundButton *btn_bki_reset_;
    std::uint8_t bki_action_key_{ 0 };

    bool allow_{ false };


public:

    LockMessageBox(QWidget *);

public:

    void allow(bool);

private:

    void update_dlg_visible();
};

