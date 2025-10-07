#pragma once

#include <vector>
#include <QAbstractTableModel>

#include <aem/json.hpp>

class ArcsListModel
    : public QAbstractTableModel
{
public:

    ArcsListModel(std::size_t = 100);

public:

    std::size_t getArcId(QModelIndex const&) const;

    // std::size_t getBaseArcId(QModelIndex const&) const;

    QString getBeginDate(QModelIndex const&) const;

    QString getProgName(QModelIndex const&) const;

public:

    void reset(std::size_t);

    inline std::size_t rowsInPage() const noexcept { return pageSize_; }

    inline bool isPageLoaded(std::size_t const pId) const noexcept { return validPages_[pId]; }

    void append(std::size_t, nlohmann::json const &);

    inline std::size_t rows() const noexcept { return arcsCount_; }

private:

    int rowCount(QModelIndex const& = QModelIndex()) const override;

    int columnCount(QModelIndex const& = QModelIndex()) const override;

    QVariant headerData(int, Qt::Orientation, int) const override;

    QVariant data(QModelIndex const&, int = Qt::DisplayRole) const override;

private:

    std::size_t arcsCount_{ 0 };
    QStringList header_;

    std::vector<QVariant> data_;
    std::vector<bool> validPages_;
    std::vector<std::size_t> aId_;

    std::size_t pageSize_;
};

