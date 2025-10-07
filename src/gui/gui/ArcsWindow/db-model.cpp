#include "db-model.h"

#include <aem/log.h>

db_model::db_model(std::string_view host, aem::uint16 port) noexcept
    : base_tcp_client(aem::time_span::zero())
    , host_(host)
    , port_(port)
{ }

void db_model::create_table_if_not_exists() 
{
    sql_ = R"(CREATE TABLE IF NOT EXISTS archive(
        id INTEGER PRIMARY KEY,
        comment TEXT,
        cg_name TEXT,
        start_time INTEGER DEFAULT 0,
        size INTEGER
      ))";

    connect(host_, port_);
    request_ = &db_model::create_new_archive_request;
}

void db_model::create_new_archive(std::string const& cg_name, std::string const& comment) noexcept
{
    sql_ = fmt::format("INSERT INTO archive(cg_name, comment, size) "
            "VALUES('{}', '{}', {})", cg_name, comment, 0);

    connect(host_, port_);
    request_ = &db_model::create_new_archive_request;
}

void db_model::delete_archive(std::size_t id) noexcept
{
    sql_ = fmt::format("DELETE FROM archive WHERE id={}", id);

    connect(host_, port_);
    request_ = &db_model::delete_archive_request;
}

void db_model::update_archive(std::size_t id, aem::int64 start_time) noexcept
{
    sql_ = fmt::format("UPDATE archive SET start_time={} WHERE id={}", start_time, id);

    connect(host_, port_);
    request_ = &db_model::update_archive_request;
}

void db_model::finalize_archive(std::size_t id, std::size_t size) noexcept
{
    sql_ = fmt::format("UPDATE archive SET size={} WHERE id={}", size, id);

    connect(host_, port_);
    request_ = &db_model::update_archive_request;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void db_model::create_new_archive_request() noexcept
{
    make_request('I', sql_, defBuf_);
    response_ = &db_model::create_new_archive_response;

    aem::log::info("db-model::create_new_archive_request: {}", sql_);

    try_send();
}

void db_model::get_in_proc_archive_request() noexcept
{
    make_request('S', sql_, defBuf_);
    response_ = &db_model::get_in_proc_archive_response;

    aem::log::info("db-model::get_in_proc_archive_request: {}", sql_);

    try_send();
}

void db_model::delete_archive_request() noexcept
{
    make_request('D', sql_, defBuf_);
    response_ = &db_model::delete_archive_response;

    aem::log::info("db-model::delete_archive_request: {}", sql_);

    try_send();
}

void db_model::update_archive_request() noexcept
{
    make_request('U', sql_, defBuf_);
    response_ = &db_model::update_archive_response;

    aem::log::info("db-model::update_archive_request: {}", sql_);

    try_send();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void db_model::create_new_archive_response(aem::uint8 const* p, aem::uint8 const* const pend) noexcept
{
    parse_response(p, pend, json_, emsg_);
    base_tcp_client::close();
}

void db_model::get_in_proc_archive_response(aem::uint8 const* p, aem::uint8 const* const pend) noexcept
{
    parse_response(p, pend, json_, emsg_);
    base_tcp_client::close();
}

void db_model::delete_archive_response(aem::uint8 const*, aem::uint8 const*) noexcept
{
    base_tcp_client::close();
}

void db_model::update_archive_response(aem::uint8 const*, aem::uint8 const*) noexcept
{
    base_tcp_client::close();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void db_model::make_request(char key, std::string_view sql, aem::buffer& buff) noexcept
{
	std::size_t const prevBufSize = buff.size();
	buff.append<aem::uint32>(0)
			.append<aem::uint8>(key)
			.append<aem::uint32>(sql.length())
			.append(sql);
	buff.cast<aem::uint32>(prevBufSize) = (buff.size() - prevBufSize);
}

bool db_model::parse_response(aem::uint8 const* p, aem::uint8 const* pend, nlohmann::json& json, std::string& emsg) noexcept
{
	// Маркер переданного сервером пакета
	char const marker = static_cast<char>(*p);
	p += sizeof(marker);

    // Положительный ответ
    if (marker == '#')
    {
        emsg.clear();
		json = nlohmann::json::from_msgpack(p, pend, true, false);
    }
    else if (marker == '$')
    {
        // Разбираем запрос, вычленяем строку запроса к БД
        if (std::distance(p, pend) >= sizeof(aem::uint32))
        {
            aem::uint32 const strlen = *(reinterpret_cast<aem::uint32 const*>(p));
            p += sizeof(aem::uint32);

            if (std::distance(p, pend) >= strlen)
                emsg = std::string(reinterpret_cast<char const*>(p), strlen);
        }

        if (emsg.empty())
            emsg = "Не удалось разобрать ответ сервера";
    }
    else
    {
        emsg = "Не удалось разобрать ответ сервера";
    }

    return emsg.empty();
}

void db_model::make_spec(std::string_view spec, aem::buffer& buff) noexcept
{
	std::size_t const prevBufSize = buff.size();
	buff.append<aem::uint32>(0)
			.append<aem::uint8>('A')
			.append<aem::uint8>(spec.length())
			.append(spec);
	buff.cast<aem::uint32>(prevBufSize) = (buff.size() - prevBufSize);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void db_model::recv_data() noexcept
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

		(this->*response_)(p, pend);

		recvBuf_.drain(rSize);
	}
}

void db_model::connection_error(std::string_view emsg) noexcept
{
    // Запоминаем ошибку
    emsg_ = emsg;
}

void db_model::connection_closed() noexcept
{
    // Вызываем обработчик ответа
    if (emsg_.empty())
    {
        if (on_done) on_done(json_);
    }
    else
    {
        if (on_error) on_error(emsg_);
    }
}

// Сразу после установки соединения специализируем обработчика нашего подключения
void db_model::connection_success() noexcept
{
    aem::log::info("db-model::connection_success");
    make_spec("db", defBuf_);
    (this->*request_)();
}

/////////////////////////////////////////////////////////////////////////////////////////////
