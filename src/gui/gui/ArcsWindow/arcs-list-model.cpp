#include "arcs-list-model.h"

#include "db-model.h"

#include <aem/environment.h>
#include <aem/log.h>

arcs_list_model::arcs_list_model() noexcept
	: QAbstractTableModel()
	, base_tcp_client(aem::time_span::s(1))
{ 
    header_ << "Дата и время\nсоздания" << "Программа" << "Размер\nархива" << "Комментарий";
    base_tcp_client::connect("127.0.0.1", aem::getenv<aem::uint16>("ARCS_DB_PORT", 5000));

    select_sql_ = "SELECT id, start_time, (CAST(strftime('%s', 'now') AS INT) - start_time) as story_time, "
        "work_desc, zrk_num, cg_name, ab_num, size, arc_name, user_name "
        "FROM archive WHERE size != 0 AND arc_name IS NOT NULL "
        "ORDER BY id DESC LIMIT {} OFFSET {}";
}

void arcs_list_model::loadPage(std::size_t min, std::size_t max) noexcept
{
    if (min >= offset_ && max < offset_ + limit_)
        return;

	if (on_response_ != nullptr)
	{
	    unhandled_page_min_ = min;
	    unhandled_page_max_ = max;
	    return;
	}

    unhandled_page_min_ = 0;
    unhandled_page_max_ = 0;

    if (min < offset_)
        next_offset_ = (max < limit_) ? 0 : max - limit_;
    else if (max >= offset_ + limit_)
        next_offset_ = min;

    db_model::make_request('S', fmt::format(select_sql_, limit_, next_offset_), defBuf_);
    on_response_ = &arcs_list_model::response_select; 
    base_tcp_client::try_send();
}

bool arcs_list_model::is_row_in_data(int row) const noexcept
{
    if (row < 0) return false;
    return ((row >= offset_) && ((row - offset_) < data_real_size_));
}

QString const& arcs_list_model::get_work_desc(int row) const noexcept
{
    return is_row_in_data(row) ? data_[row - offset_][8] : sstub_;
}

QString const& arcs_list_model::get_cg_name(int row) const noexcept
{
    return is_row_in_data(row) ? data_[row - offset_][7] : sstub_;
}

QString const& arcs_list_model::get_ab_num(int row) const noexcept
{
    return is_row_in_data(row) ? data_[row - offset_][2] : sstub_;
}

QString const& arcs_list_model::get_zrk_num(int row) const noexcept
{
    return is_row_in_data(row) ? data_[row - offset_][1] : sstub_;
}

QString const& arcs_list_model::get_user_name(int row) const noexcept
{
    return is_row_in_data(row) ? data_[row - offset_][3] : sstub_;
}

QString const& arcs_list_model::get_arc_name(int row) const noexcept
{
    return is_row_in_data(row) ? data_[row - offset_][5] : sstub_;
}

QString const& arcs_list_model::get_cg_type(int row) const noexcept
{
    // return is_row_in_data(row) ? data_[row - offset_][8] : sstub_;
    return sstub_;
}

std::size_t arcs_list_model::get_arc_id(int row) const noexcept
{
    return is_row_in_data(row) ? rid_[row - offset_] : 0;
}

int arcs_list_model::rowCount(QModelIndex const&) const noexcept
{
	return arcs_count_;
}

int arcs_list_model::columnCount(QModelIndex const&) const noexcept
{
	return header_.size(); 
}

QVariant arcs_list_model::data(QModelIndex const& index, int role) const noexcept
{
	int const row = index.row();
	int const col = index.column();

    // Инициируем загрузку нового диапазона архивов
	if (!is_row_in_data(row))
	    return QVariant();

	switch (role)
	{
	case Qt::DisplayRole: 
	    return data_[row - offset_][col];

	case Qt::TextAlignmentRole:
	    return Qt::AlignCenter;
	}

	return QVariant();
}

QVariant arcs_list_model::headerData(int section, Qt::Orientation orientation, int role) const noexcept
{
	if (orientation == Qt::Vertical)
	    return QVariant();

	switch(role)
	{
	case Qt::DisplayRole:
		return header_[section];
	}

	return QVariant();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void arcs_list_model::recv_data() noexcept
{
	// Разбираем прочитанные данные
	while (!recvBuf_.empty())
	{
		// Проверяем наличие в буффере необходимого количества данных
		if (recvBuf_.size() < sizeof(aem::uint32))
			break;
		aem::uint8 const* p = recvBuf_.begin();

		// Размер переданного сервером пакета
		aem::uint32 rSize = *(reinterpret_cast<aem::uint32 const*>(p));
		if (recvBuf_.size() < rSize)
			break;

		aem::uint8 const* const pend = p + rSize;
		p += sizeof(rSize);

		(this->*on_response_)(p, pend);

		recvBuf_.drain(rSize);
	}
}

void arcs_list_model::connection_error(std::string_view emsg) noexcept
{
    aem::log::error("arcs-list-model::connection_error: {}", emsg);
}

void arcs_list_model::connection_closed() noexcept
{
    aem::log::trace("arcs-list-model::connection_closed");
}

void arcs_list_model::connection_success() noexcept
{
    aem::log::trace("arcs-list-model::connection_success");

    db_model::make_spec("db", defBuf_);
    db_model::make_request('S', "SELECT COUNT(*) FROM archive WHERE size != 0", defBuf_);
    on_response_ = &arcs_list_model::response_count; 

    base_tcp_client::try_send();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void arcs_list_model::response_count(aem::uint8 const* p, aem::uint8 const* pend) noexcept
{
    nlohmann::json json;
    std::string emsg;

    if (!db_model::parse_response(p, pend, json, emsg))
    {
        aem::log::error("arcs-list-model::response_count: {}", emsg);
        return;
    }

    aem::log::trace("arcs-list-model::response_count: done: {}", json.dump());

    arcs_count_ = std::stoul(json[0]["COUNT(*)"].get_ref<std::string const&>());
    if (arcs_count_ != 0)
    {
        beginInsertRows(QModelIndex(), 0, arcs_count_ - 1);
        endInsertRows();
    }

    db_model::make_request('S', fmt::format(select_sql_, limit_, offset_), defBuf_);
    on_response_ = &arcs_list_model::response_select; 
    base_tcp_client::try_send();
}

static QString up_story_time(std::string const &v)
{
    std::size_t story_time = 0;
    try {
	    story_time = std::stoull(v);
	}
	catch (...) {
	}

    char buf[128] = {};
    std::snprintf(buf, sizeof(buf), "%zu дн.", story_time / (3600 * 24));

    return QString(buf);
}

static QString up_datetime(std::string const &v)
{
    std::time_t unix_time = 0;
    try {
	    unix_time = std::stoull(v);
	}
	catch (...) {
	}

    char buf[128] = {};

	std::tm *to_now = std::localtime(&unix_time);
	if (to_now)
		std::strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", to_now);

	return QString(buf);
}

static QString up_size(std::string const& bytes)
{
    double out_v = 0;
    try {
	    out_v = std::stod(bytes);
	}
	catch (...) {
	}

	int meas_id = 0;
	char const* const meas[] = {"байт", "Кб", "Мб", "Гб"};

	for(int i = 0; i < std::size(meas); ++i)
	{
		if (out_v < 1024.0)
			break;
		out_v /= 1024;
		meas_id += 1;
	}

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f %s", out_v, meas[meas_id]);

    return QString(buf);
}

std::string string_from_json(nlohmann::json const& json) noexcept
{
    try
    {
        return json.get_ref<std::string const&>(); 
    }
    catch(std::exception const&)
    {
        return "";
    }
}

void arcs_list_model::response_select(aem::uint8 const* p, aem::uint8 const* pend) noexcept
{
    on_response_ = nullptr;

    nlohmann::json json;
    std::string emsg;

    if (!db_model::parse_response(p, pend, json, emsg))
    {
        aem::log::error("arcs-list-model::response_spec_done: {}", emsg);
        return;
    }

    offset_ = next_offset_;

    aem::log::trace("arcs-list-model::response_spec_done: {}", json.dump());

    data_real_size_ = 0;
	for (auto const& row : json)
	{
        rid_[data_real_size_] = std::stoul(row["id"].get_ref<std::string const&>());

	    auto &drec = data_[data_real_size_++];

	    // Дата создания
	    drec[0] = up_datetime(row["start_time"].get_ref<std::string const&>());
	    // Номер ЗРК
        std::size_t zrk_num = std::stoul(row["zrk_num"].get_ref<std::string const&>());
	    drec[1] = QString::number(zrk_num + 1);
	    // Номер АБ
	    drec[2] = QString::fromStdString(row["ab_num"].get_ref<std::string const&>());
	    // Оператор
	    drec[3] = QString::fromStdString(row["user_name"].get_ref<std::string const&>());
	    // Время хранения
	    drec[4] = up_story_time(string_from_json(row["story_time"]));
	    // Имя архива
	    drec[5] = QString::fromStdString(row["arc_name"].get_ref<std::string const&>());
	    // Размер
	    drec[6] = up_size(row["size"].get_ref<std::string const&>());

	    // Имя ЦГ
	    drec[7] = QString::fromStdString(row["cg_name"].get_ref<std::string const&>());
	    // Назначение работы
	    drec[8] = QString::fromStdString(row["work_desc"].get_ref<std::string const&>());
	}

	if (data_real_size_ != 0)
        dataChanged(createIndex(offset_, 0), createIndex(offset_ + data_real_size_ - 1, header_.size() - 1));

    // Если есть необработанный диапазон
    if (unhandled_page_min_ != unhandled_page_max_)
        loadPage(unhandled_page_min_, unhandled_page_max_);
}










