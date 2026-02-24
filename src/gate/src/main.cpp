#include <aem/utils/udev_notify.h>

#include <array>
#include <blkid/blkid.h>
#include <exception>
#include <iterator>
#include <sys/mount.h>

#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <charconv>

#include <aem/aem.h>
#include <aem/types.h>

#include <fmt/format.h>

// Каталог с точками монтирования
std::filesystem::path dpath = "/tmp/media";

// Список на данный момент примонтированных устройств
std::map<std::string, std::filesystem::path> mount_points;

// При наличии прошивки обновлять автоматически
bool auto_update = true;

std::string id_sid;
std::filesystem::path tmp_directory = "/tmp/root";
std::filesystem::path rw_directory = "/home/root";

struct update_info
{
	std::time_t timestamp;
	std::size_t version;
};

std::pair<std::string, std::size_t> current_update;
std::size_t current_versions;

bool mount_new_partition(blkid_cache cache, std::string const& partition)
{
	blkid_dev dev = blkid_get_dev(cache, partition.c_str(), BLKID_DEV_NORMAL);
	if (dev == nullptr)
	{
		aem::log::error("[{}]: Не удалось получить информацию об разделе\n", partition);
		return false;
	}

	std::string uuid;
	std::string fs_type;
	std::string label;

	blkid_tag_iterate iter = blkid_tag_iterate_begin(dev);
	char const *type, *value;
	while (blkid_tag_next(iter, &type, &value) == 0)
	{
		if (!strcmp(type, "UUID"))
		{
			aem::log::info("[{}]: UUID = {}\n", partition, value);
			uuid = value;
		}
		if (!strcmp(type, "TYPE"))
		{
			aem::log::info("[{}]: TYPE = {}\n", partition, value);
			fs_type = value;
		}
		if (!strcmp(type, "LABEL"))
		{
			aem::log::info("[{}]: LABEL = {}\n", partition, value);
			label = value;
		}
	}
	blkid_tag_iterate_end(iter);

	// Если у нас нет файловой системы, выходим
	if (fs_type.empty())
	{
		aem::log::warn("[{}]: На разделе нет файловой системы\n", partition);
		return false;
	}

	// Если есть метка, пробуем использовать ее в качестве имени точки монтирования
	std::string mount_point_name = label;
	if (!mount_point_name.empty())
	{
		// Проверяем, может с таким именем уже есть
		std::error_code ec;
		bool result = std::filesystem::exists(dpath / label, ec);
		if (ec)
		{
			aem::log::warn("[{}]: Не удалось проверить наличие точки монтирования с именем \"{}\" - {}\n", label, ec.message());
			result = true;
		}

		// Если мы не можем использовать метку в качестве имени, используем UUID
		if (result)
			mount_point_name = uuid;
	}
	else
	{
		mount_point_name = uuid;
	}

	std::error_code ec;
	bool result = std::filesystem::exists(dpath / mount_point_name, ec);
	if (ec)
	{
		aem::log::warn("[{}]: Не удалось проверить наличие точки монтирования с именем \"{}\" - {}\n", mount_point_name, ec.message());
		result = true;
	}

	if (result)
	{
		aem::log::error("[{}]: Не удалось примонтировать раздел, проблема с именем точки монтирования: {}\n", partition, mount_point_name);
		return false;
	}

	std::filesystem::path mp = dpath / mount_point_name;

	std::filesystem::create_directory(mp, ec);
	if (ec)
	{
		aem::log::error("[{}]: Не удалось создать точку монтирования \"{}\" - {}\n", partition, mp.string(), ec.message());
		return false;
	}

	if (fs_type == "ntfs")
	{
		int result = std::system(fmt::format("ntfs-3g {} {}", partition, mp.string()).c_str());
		aem::log::warn("ntfs-3g: {}\n", result);
		if (result == 0)
		{
			aem::log::info("[{}]: Раздел примонтирован в \"{}\", файловая система: {}\n", partition, mp.string(), fs_type);
			mount_points[partition] = mp;

			return true;
		}
	}
	else
	{
		if (::mount(partition.c_str(), mp.c_str(), fs_type.c_str(), MS_SYNCHRONOUS, nullptr) == 0)
		{
			aem::log::info("[{}]: Раздел примонтирован в \"{}\", файловая система: {}\n", partition, mp.string(), fs_type);
			mount_points[partition] = mp;

			return true;
		}
	}

	aem::log::error("[{}]: Не удалось примонтировать раздел - {}\n", partition, strerror(errno));

	return false;
}

void check_partition_unmounted(std::string const& partition)
{
	// Устройство было извлечено, необходимо очистить каталог точек монтирования
	auto it = mount_points.find(partition);

	// Мы такого раздела не монтировали
	if (it == mount_points.end())
		return;

	// Пробуем отмонтировать
	if (::umount(it->second.c_str()) == -1)
		aem::log::error("[{}]: Не удалось отмонтировать устройство - {}\n", partition, strerror(errno));

	// Удаляем точку монтирования
	std::error_code ec;
	std::filesystem::remove(it->second, ec);

	if (ec)
		aem::log::error("[{}]: Не удалось удалить точку монтирования \"{}\" - {}\n", partition, it->second.string(), ec.message());

	mount_points.erase(it);
}

	struct helper
	{
		static std::size_t check_version(std::string_view v)
		{
		 	std::size_t version = 0;

		 	int const base = 16;
		 	auto result = std::from_chars(v.data(), v.data() + v.length(), version, base);

		 	if (result.ptr != (v.data() + v.length()) || result.ec != std::errc())
		 		throw std::invalid_argument(fmt::format("[{}]: Версия говно\n", v));

		 	return version;
		}

		static std::time_t check_timestamp(std::string_view t)
		{
			std::tm tm = {};
		    std::istringstream ss{std::string{t}};

		    ss >> std::get_time(&tm, "%Y-%m-%d");
		    if (ss.fail() || not ss.eof())
		    	throw std::invalid_argument(fmt::format("[{}]: Дата говно\n", t));

		    return std::mktime(&tm);
		}
	};

bool parse_update_file_name(std::string_view filename, update_info& info) noexcept
{
    try
    {
        constexpr std::string_view pattern = "xxxxxx.yyyy-mm-dd.ext";
        constexpr auto version_end = pattern.find('.');
        constexpr auto timestamp_end = pattern.find('.', version_end + 1);

        if (filename.length() < pattern.length())
        {
        	aem::log::trace("[{}]: Слишком короткое имя файла\n", filename);
            return false;
        }

        if (filename[version_end] != '.' ||
            filename[timestamp_end] != '.')
        {
        	aem::log::trace("[{}]: Не верный формат имени файла\n", filename);
            return false;
        }

        if (filename.substr(timestamp_end + 1) != "tgz")
        {
        	aem::log::trace("[{}]: Не верный тип файла: [{}]\n", filename, filename.substr(timestamp_end + 1));
            return false;
        }

        info.version 	= helper::check_version(filename.substr(0, version_end));
        info.timestamp 	= helper::check_timestamp(filename.substr(version_end + 1, timestamp_end - version_end - 1));

        return true;
    }
    catch (std::exception const& e)
    {
    	aem::log::trace("[{}]: {}\n", filename, e.what());
        return false;
    }
}

void update_current_version() noexcept
{
	std::filesystem::path path("/root/id.version");

	if (not std::filesystem::exists(path))
		return;

	// Читаем содержимое файла
	auto file = std::ifstream{path.string()};
	std::string version{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>()};

    constexpr std::string_view pattern = "xxxxxx.yyyy-mm-dd";
    constexpr auto version_end = pattern.find('.');

	current_versions = helper::check_version(version.substr(0, version_end));
}


bool scan_partition_for_update_files(std::string const& partition) noexcept
{
	aem::log::info("[{}]: Сканируем раздел на наличие прошивок\n", partition);

	auto const it = mount_points.find(partition);
	if (it == mount_points.end())
	{
		aem::log::error("[{}]: Какая-то ерунда при попытке начать сканировать раздел\n", partition);
		return false;
	}

	std::filesystem::path source_directory = it->second / id_sid;
	if (not std::filesystem::is_directory(source_directory))
	{
		aem::log::info("[{}]: На разделе отсутствует требуемый каталог [{}]\n", partition, id_sid);
		return false;
	}

	update_info ui;


	// Перебираем все файлы в поисках наиболее подходящего
    for (auto const& dir_entry : std::filesystem::directory_iterator{source_directory})
    {
    	if (not dir_entry.is_regular_file())
    	{
    		aem::log::info("[{}]: Не файл [{}]\n", partition, dir_entry.path().c_str());
    		continue;
    	}

    	std::string filename(dir_entry.path().filename());
    	if (not parse_update_file_name(filename.c_str(), ui))
    	{
    		aem::log::info("[{}]: Не удалось получить информацию о файле прошивки [{}]\n", partition, filename);
    		continue;
    	}

    	auto& item = current_update;
    	if (not item.first.empty() and item.second >= ui.version)
    	{
    		aem::log::info("[{}]: Ранее найдена более свежая версия [{}]\n", partition, filename);
    		continue;
    	}

    	if (ui.version <= current_versions)
    	{
    		aem::log::info("[{}]: Текущая версия более свежая [{}]\n", current_versions, ui.version);
    		continue;
    	}

    	item = std::make_pair(filename, ui.version);

		if (auto_update)
		{
			// Копируем сразу куда надо
			std::filesystem::path dst = rw_directory / id_sid;
			std::filesystem::create_directories(dst);

			aem::log::info("[{}]: Обнаружена новая прошивка [{}]\n", partition, filename);
			std::filesystem::copy_file(source_directory / filename, dst / "firmware.tgz", std::filesystem::copy_options::overwrite_existing);

			current_versions = ui.version;

			return true;
		}
    }

	return false;
}

int main(int argc, char const* argv[])
{
	if (argc != 2)
	{
		aem::log::error("Необходимо указать идентификатор проекта\n");
		return -1;
	}

	id_sid = argv[1];

	update_current_version();

	blkid_cache cache;

	int result = blkid_get_cache(&cache, nullptr);
	if (result != 0)
	{
		aem::log::error("Не удалось инициализировать blkid cache\n");
		return -1;
	}

	// Создаем место, куда будем монтировать все устройства
	std::error_code ec;
	std::filesystem::create_directories(dpath, ec);
	if (ec)
	{
		aem::log::error("Не удалось создать каталог [{}] для точек монтирования - {}\n", dpath.string(), ec.message());
		return -1;
	}

	// Инициализируем udev

	aem::utils::udev_notify udev(aem::utils::udev_notify::NetLinkId::UDev);

	udev.add_watch(aem::utils::udev_notify::SubSystemId::Block, aem::utils::udev_notify::TypeId::Partition,
		[&](aem::utils::udev_notify::ActionId ac, std::string_view node, aem::utils::udev_notify::SubSystemId ss, aem::utils::udev_notify::TypeId ti)
		{
			std::string partition(node);

			switch(ac)
			{
			case aem::utils::udev_notify::ActionId::Add:
				if (!mount_new_partition(cache, partition))
					return;
				if (scan_partition_for_update_files(partition))
				{
					int result = std::system("reboot");
					aem::log::warn("Перезагружаемся: {}\n", result);
				}
				break;
			case aem::utils::udev_notify::ActionId::Remove:
				check_partition_unmounted(partition);
				break;
			default:
				aem::log::warn("Необрабатываемое событие: {}\n", aem::utils::udev_notify::actionName(ac));
			}
		});

	udev.start();

	return aem::run();
}