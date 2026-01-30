#pragma once

#include <filesystem>

#include <QAbstractTableModel>
#include <QStringList>

#include "ambit/common/program.hpp"

class ProgramModel
    : public QAbstractTableModel
{
public:

    ProgramModel();

public:

    static bool load_program_comments(QString, QString &) noexcept;

public:

    bool save_to_file() const noexcept;

    bool load_from_local_file(QString) noexcept;

    bool load_from_usb_file(QString const&) noexcept;

public:

    program const& prog() const noexcept { return program_; }

    void set_edited_row(std::size_t row) noexcept {
        edited_row_ = row;
    }

    void clear_current_row();

    void set_current_row(std::size_t) noexcept;

    void set_current_phase(std::size_t) noexcept;

    void change_sprayer(std::size_t, std::size_t) noexcept;

    void change_main(std::size_t, std::size_t, std::size_t, double) noexcept;

    void change_pause(std::size_t, std::uint64_t) noexcept;

    void change_loop(std::size_t, std::size_t, std::size_t) noexcept;

    void change_fc(std::size_t, float, float, float) noexcept;

    void change_center(std::size_t, centering_type, float) noexcept;

    void add_op(program::op_type) noexcept;

    void add_main_op(bool) noexcept;

    void remove_op() noexcept;

    std::size_t last_insert_row() const noexcept;

    std::string get_base64_program() const noexcept;

    QString const& name() const noexcept { return name_; } 

    QString const& comments() const noexcept { return comments_; }

    void set_name(QString const& name) noexcept { name_ = name; } 

    void set_comments(QString const& comments) noexcept { comments_ = comments; }

    void reset() noexcept;

private:

    int rowCount(QModelIndex const& = QModelIndex()) const override;

    int columnCount(QModelIndex const& = QModelIndex()) const override ;

    QVariant headerData(int, Qt::Orientation, int) const override { return QVariant(); }

    QVariant data(QModelIndex const&, int = Qt::DisplayRole) const override;

    bool data_main_op_equal(std::size_t, std::size_t) const;

    QString data_main_op_text(std::size_t, std::size_t) const;

private:

    bool load_from_file(QString const&) noexcept;

private:

    program program_;

    QString name_;
    QString comments_;

    std::size_t edited_row_{ 0 };
    std::optional<std::size_t> current_row_;

    std::filesystem::path path_;
};

