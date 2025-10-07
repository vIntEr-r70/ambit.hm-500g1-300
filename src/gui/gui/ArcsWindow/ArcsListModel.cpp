#include "ArcsListModel.h"

#include <QColor>
#include <QStringList>

#include <aem/log.h>

//! Цвета записей в таблице:
//! цвет фона: обычный архив
//! синеватый: базовый архив
//! красноватый: верификация не прошла
//! зеленоватый: верификация прошла

ArcsListModel::ArcsListModel(std::size_t pageSize)
    : QAbstractTableModel()
    , pageSize_(pageSize)
{ 
    header_ << "Дата и время\nсоздания" << "Программа" << "Размер\nархива" << "Комментарий";
}

void ArcsListModel::reset(std::size_t arcsCount)
{
    beginResetModel();
    arcsCount_ = arcsCount;
    validPages_.clear();
    validPages_.resize((arcsCount_ + pageSize_) / pageSize_, false);
    aId_.clear();
    aId_.resize(arcsCount_, 0);
    data_.resize(arcsCount_ * header_.size());
    endResetModel();
}

void ArcsListModel::append(std::size_t const pId, nlohmann::json const &resp)
{
    beginResetModel();

    std::size_t const rowId = pId * pageSize_;
    for (std::size_t row = 0; row < resp.size(); ++row)
    {
        auto const& rec = resp[row];
        
        //! 0 - индекс архива
        aId_[rowId + row] = rec[0].get<std::size_t>();

        std::size_t dB = (rowId + row) * header_.size();
        std::size_t cIdx = 0;

        //! Дата и время создания архива в UTC
        data_[dB + cIdx++] = QString::fromStdString(rec[1].get_ref<std::string const &>());
        //! Название программы, по которой проводилось испытание
        data_[dB + cIdx++] = QString::fromStdString(rec[2].get_ref<std::string const &>());
        //! Размер архива в байтах
        data_[dB + cIdx++] = rec[3].get<qlonglong>();
        //! Информация об архиве 
        data_[dB + cIdx++] = QString::fromStdString(rec[4].get_ref<std::string const &>());
    }
    validPages_[pId] = true;

    endResetModel();
}

int ArcsListModel::rowCount(QModelIndex const&) const
{
    return arcsCount_;
}

int ArcsListModel::columnCount(QModelIndex const&) const
{
    return header_.count();
}

QVariant ArcsListModel::headerData(int cId, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    return header_[cId];
}

QVariant ArcsListModel::data(QModelIndex const& index, int role) const
{
    std::size_t rId = index.row();
    std::size_t cId = index.column();

    //! Если данные еще не готовы
    if (!validPages_[rId / pageSize_])
        return QVariant();

    switch (role)
    {
    case Qt::TextAlignmentRole:
        return (cId != 2) ? Qt::AlignCenter : static_cast<int>((Qt::AlignLeft | Qt::AlignVCenter));

    case Qt::DisplayRole:
        return data_[rId * header_.size() + cId];
    default:
        return QVariant();
    }
}

std::size_t ArcsListModel::getArcId(QModelIndex const& index) const
{
    return aId_[index.row()];
}

// std::size_t ArcsListModel::getBaseArcId(QModelIndex const& index) const
// {
//     return aId_[index.row()].second;
// }

QString ArcsListModel::getBeginDate(QModelIndex const& index) const
{
    std::size_t offset = index.row() * header_.size();
    return data_[offset].toString();
}

QString ArcsListModel::getProgName(QModelIndex const& index) const
{
    std::size_t offset = index.row() * header_.size();
    return data_[offset + 1].toString();
}
