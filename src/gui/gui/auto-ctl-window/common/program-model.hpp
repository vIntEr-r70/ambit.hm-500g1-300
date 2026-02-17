#pragma once

#include "common/program.hpp"

#include <QAbstractTableModel>

class program_model
    : public QAbstractTableModel
{
protected:

    program program_;

    std::size_t current_row_;

public:

    program_model() = default;

public:

    program const& prog() const noexcept { return program_; }

    void set_program(program);

    std::size_t current_row() const noexcept { return current_row_; }

private:

    int rowCount(QModelIndex const& = QModelIndex()) const override;

    int columnCount(QModelIndex const& = QModelIndex()) const override ;

    QVariant headerData(int, Qt::Orientation, int) const override { return QVariant(); }

    QVariant data(QModelIndex const&, int = Qt::DisplayRole) const override;

    bool data_main_op_equal(std::size_t, std::size_t) const;

    QString data_main_op_text(std::size_t, std::size_t) const;

private:

    virtual std::size_t loop_repeat_count(std::size_t, std::size_t N) const { return N; }
};

