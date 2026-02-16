#pragma once

#include "editor-page-header-widget.hpp"
#include "common/program.hpp"

#include <eng/sibus/node.hpp>

#include <filesystem>

class AutoParamKeyboard;
class program_widget;
class program_model_editor;
class EditorMessageBox;

struct program_record_t;

class editor_page final
    : public QWidget
    , public eng::sibus::node
{
    Q_OBJECT

    program_model_editor *model_;
    program_widget *program_widget_;

    editor_page_header_widget *header_;
    AutoParamKeyboard *kb_;

    EditorMessageBox *msg_;

    std::unordered_map<char, double> positions_;

    program empty_program_;

    std::filesystem::path path_;
    std::string fname_;

public:

    editor_page(QWidget *);

signals:

    void make_done();

    void make_load(std::string const &);

public:

    void init(program_record_t const *);

private:

    void make_save();

    void make_play();

    void make_exit();

private:

    void make_table_op(editor_page_header_widget::TableAc);

    void table_cell_select(QModelIndex);

    bool save_to_file() const;
};
