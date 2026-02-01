#pragma once

#include <InteractWidgets/InteractWidget.h>

#include "common/program.hpp"


class QLabel;
class QLineEdit;
class RoundButton;
class QStackedWidget;
class QDoubleValidator;
class ValueSetBool;

class AutoParamKeyboard 
    : public InteractWidget 
{
    Q_OBJECT

    typedef std::function<void(float)> main_accept_cb_t;
    typedef std::function<void(std::uint64_t)> pause_accept_cb_t;
    typedef std::function<void(std::size_t, std::size_t)> loop_accept_cb_t;
    typedef std::function<void(float, float, float)> fc_accept_cb_t;
    typedef std::function<void(centering_type, float)> center_accept_cb_t;

public:

    AutoParamKeyboard(QWidget*);

public:

    void show_main(QString const&, QString const&, float, float, main_accept_cb_t &&);

    void show_main(QString const&, QString const&, float, float, float, main_accept_cb_t &&);

    void show_pause(std::uint64_t, pause_accept_cb_t &&);

    void show_loop(std::size_t, std::size_t, loop_accept_cb_t &&);

    void show_fc(float, float, float, fc_accept_cb_t &&);

    void show_center(centering_type, float, center_accept_cb_t &&);

private slots:

    void onKeyPressed(int);

    void onACKey();

    void onBackspaceKey();

private:

    void showEvent(QShowEvent*) override final;

private:

    void on_le_changed();

    void update_accept_button(bool);

    void update_center_type() noexcept;

private:

    QWidget* create_main_page();

    QWidget* create_pause_page();

    QWidget* create_fc_page();

    QWidget* create_loop_page();

    QWidget* create_center_page();

    QLineEdit* create_line_edit(QWidget*);

private:

    void main_accept_handler();
    void main_acc_handler();

    void pause_accept_handler();
    void pause_focus_handler();

    void loop_accept_handler();

    void fc_accept_handler();

    void center_accept_handler();

private:

    QLabel *title_;

    struct
    {
        QWidget *w;
        QDoubleValidator *validator;
        QLineEdit* le;

    } main_page_;

    struct
    {
        QWidget *w;
        QDoubleValidator *validator[3];
        QLineEdit* le[3];

    } pause_page_;

    struct
    {
        QWidget *w;
        QDoubleValidator *validator[3];
        QLineEdit* le[3];

    } fc_page_;

    struct
    {
        QWidget *w;
        QDoubleValidator *validator[2];
        QLineEdit* le[2];

    } loop_page_;

    struct
    {
        QWidget *w;
        QDoubleValidator *validator[2];
        ValueSetBool *type;
        ValueSetBool *tooth;
        QLineEdit* le;

    } center_page_;

    std::vector<QLineEdit*> le_;

    QStackedWidget *stack_;

    RoundButton *btn_accept_;

    std::map<int, RoundButton*> buttons_;

    main_accept_cb_t main_accept_cb_;
    pause_accept_cb_t pause_accept_cb_;
    loop_accept_cb_t loop_accept_cb_;
    fc_accept_cb_t fc_accept_cb_;
    center_accept_cb_t center_accept_cb_;

    void (AutoParamKeyboard::*on_accept_handler_)();
    void (AutoParamKeyboard::*on_focus_handler_)();
    void (AutoParamKeyboard::*on_acc_handler_)();
};



