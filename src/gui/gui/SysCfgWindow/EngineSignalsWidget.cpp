#include "EngineSignalsWidget.h"

#include <QTableView>
#include <QAbstractTableModel>
#include <QVBoxLayout>
#include <QHeaderView>

#include "global.h"

#include <aem/log.h>
#include <aem/json.hpp>

class SignalsModel 
    : public QAbstractTableModel
{
public:

    SignalsModel() noexcept
		: QAbstractTableModel()
	{ }

private:

    int rowCount(QModelIndex const&) const noexcept override final
	{
	    return rows_.size(); 
	}

    int columnCount(QModelIndex const&) const noexcept override final
	{
		return 5; 
	}

    QVariant data(QModelIndex const& index, int role) const noexcept override final
	{
	    int const row = index.row();
	    int const col = index.column();

	    switch (role)
	    {
	    case Qt::DisplayRole:
		    return rows_[row][col];
	    case Qt::TextAlignmentRole:
	        return (col == 1 || col == 2 || col == 3) ? Qt::AlignCenter : Qt::AlignVCenter;
	    }

	    return QVariant();
	}

    QVariant headerData(int section, Qt::Orientation orientation, int role) const noexcept override final
	{
	    if (orientation == Qt::Vertical)
	        return QVariant();

	    static QStringList const header{ "Тип", "Источник", "Параметр", "#", "Описание" };

		switch(role)
		{
		case Qt::DisplayRole:
			return header[section];
		}

		return QVariant();
	}

public:

    void set_data(nlohmann::json::array_t const& json) noexcept
    {
        if (!rows_.empty())
        {
            beginRemoveRows(QModelIndex(), 0, rows_.size() - 1);
            rows_.clear();
            endRemoveRows();
        }

        if (json.empty())
            return;
        
        try
        {
	        for (auto const& row : json)
	        {
	            auto it_uid = row.find("uid");
	            if (it_uid != row.end())
	            {
                    std::size_t uid = it_uid->get<std::size_t>();
                    suid_[uid] = rows_.size();
	            }

                auto it_info = row.find("info");

	            rows_.push_back({ 
	                QString::fromStdString(row.at("key").get_ref<std::string const&>()),
	                QString::fromStdString(row.at("source").get_ref<std::string const&>()),
	                QString::fromStdString(row.at("param").get_ref<std::string const&>()),
	                "---",
	                (it_info != row.end()) ? QString::fromStdString(it_info->get_ref<std::string const&>()) : ""
	            });
	        }

	        beginInsertRows(QModelIndex(), 0, rows_.size() - 1);
	        endInsertRows();
        }
        catch(...)
        {
            aem::log::error("EngineSignalsWidget::set_data: {}", nlohmann::json(json).dump());
        }
    }

    void touch(std::size_t uid, std::size_t amount) noexcept
    {
        auto it = suid_.find(uid);
        if (it == suid_.end())
            return;
        rows_[it->second][3] = QString::number(amount);
        dataChanged(createIndex(it->second, 3), createIndex(it->second, 3));
    }

private:

    std::map<std::size_t, std::size_t> suid_;
    std::vector<std::array<QString, 5>> rows_;
};

EngineSignalsWidget::EngineSignalsWidget(QWidget *parent) noexcept
    : QWidget(parent)
    , rpc_(global::rpc())
{
    model_ = new SignalsModel();

    QVBoxLayout *vL = new QVBoxLayout(this);
    {
        table_ = new QTableView(this);
        table_->setModel(model_);
        table_->setFocusPolicy(Qt::NoFocus);
        table_->setSelectionMode(QAbstractItemView::NoSelection);
	    table_->horizontalHeader()->setStretchLastSection(true);
	    table_->horizontalHeader()->resizeSection(0, 180);
	    table_->horizontalHeader()->resizeSection(1, 100);
	    table_->horizontalHeader()->resizeSection(2, 100);
	    table_->horizontalHeader()->resizeSection(3, 50);
	    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	    table_->verticalHeader()->setDefaultSectionSize(30);
	    table_->verticalHeader()->hide();
	    table_->setAlternatingRowColors(true);
        vL->addWidget(table_);
    }

    rpc_.call("load-engine-signals")
        .done([this](nlohmann::json const &args)
        {
            model_->set_data(args);

            // global::subscribe("signals.{}.amount", [this](nlohmann::json::array_t const& keys, nlohmann::json const& value)
            // {
            //     std::size_t uid = keys[0].get<std::size_t>();
            //     model_->touch(uid, value.get<std::size_t>());
            // });
        })
        .error([this](std::string_view emsg)
        {
            aem::log::error("EngineSignalsWidget: load-engine-signals: {}", emsg);
        });

}


