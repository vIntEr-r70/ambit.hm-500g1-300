#pragma once

#include <QFrame>

class QTableView;
class QTextEdit;
class QLabel;

class arcs_list_model;
class text_output;
class file_system_dlg;
class main_header;

class arcs_list_page final
    : public QFrame
{
    Q_OBJECT

public:

    arcs_list_page(QWidget*) noexcept;

public:

    void close_model() noexcept;

    void set_cg_mode(char const*) noexcept;

signals:

    void load_arc(std::size_t);

    void do_remove(std::size_t);

    void do_export();

    void do_copy();

private slots:

    void on_row_changed(QModelIndex const&, QModelIndex const&) noexcept;

    void onVerticalValueChanged(int) noexcept;

private:

    void on_open_arc() noexcept;
    
private:

    arcs_list_model *model_;
    /* main_header &header_; */

    QTableView *table_;
    QTextEdit *work_desc_;

    QLabel *cg_name_;
    QLabel *cg_type_;

    int irow_{ -1 };
};


