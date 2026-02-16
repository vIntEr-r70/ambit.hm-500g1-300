#include "program-list-model.hpp"
#include "../common/program-record.hpp"

#include <eng/log.hpp>
#include <eng/read-cast.hpp>
#include <eng/crc.hpp>

#include <fcntl.h>
#include <filesystem>
#include <qvariant.h>
#include <uv.h>

#include <bitset>

struct context_t
{
    context_t(program_list_model &ref)
        : self(ref)
    {
        read.data = this;
        copy.data = this;
        event.data = this;
    }

    program_list_model &self;

    std::filesystem::path path;

    std::vector<std::string> dirty_files;

    std::vector<std::filesystem::path> syncing_files;
    std::vector<std::string> removing_files;

    std::string process_file;

    uv_fs_event_t event;
    uv_fs_t read;
    uv_fs_t copy;

    uv_dir_t *dir;
    std::array<uv_dirent_t, 16> dirent_storage;

    std::list<program_record_t> file;
    uv_file file_handle;

    std::size_t bit_set;
};

namespace
{

    static std::list<program_record_t> cache;

    constexpr static program_record_t& alloc_record(context_t &ctx)
    {
        // eng::log::info("{}", __func__);

        if (cache.empty())
            cache.emplace(cache.end());

        if (!ctx.file.empty())
            throw std::runtime_error("file list in context is not empty");

        ctx.file.splice(ctx.file.begin(), cache, cache.begin());

        return ctx.file.front();
    }

    constexpr static void free_record(context_t &ctx)
    {
        // eng::log::info("{}", __func__);
        cache.splice(cache.begin(), ctx.file, ctx.file.begin());
    }

    constexpr static void free_record(std::list<program_record_t> &list, program_record_t &r)
    {
        // eng::log::info("{}", __func__);
        cache.splice(cache.begin(), list, r.iterator);
    }

}


program_list_model::program_list_model()
    : QAbstractTableModel()
{
    header_ << "Название" << "Дата" << "" << "";

    icon_hd_ = QIcon(":/file.folder-open");
    icon_usb_ = QIcon(":/file.usb");
}

void program_list_model::add_local_path(std::filesystem::path path)
{
    hd_ = std::make_unique<context_t>(*this);
    hd_->path = std::move(path);
    hd_->bit_set = program_record_t::hd_bit;

    start_monitoring_directory(*hd_);
}

void program_list_model::add_usb_path(std::filesystem::path path)
{
    usb_ = std::make_unique<context_t>(*this);
    usb_->path = std::move(path);
    usb_->bit_set = program_record_t::usb_bit;

    start_monitoring_directory(*usb_);
}

program_record_t const* program_list_model::record(std::size_t row) const
{
    return records_index_map_[row];
}

program_record_t const* program_list_model::record(std::string const &fname) const
{
    auto it = std::ranges::find_if(records_index_map_, [&](auto const &item) {
        return item->filename == fname;
    });
    return it == records_index_map_.end() ? nullptr : *it;
}

void program_list_model::start_monitoring_directory(context_t &ctx)
{
    // eng::log::info("{}: {}", __func__, ctx.path.native());

    uv_fs_event_init(uv_default_loop(), &ctx.event);
    uv_fs_event_start(&ctx.event,
            [](uv_fs_event_t *handler, const char *filename, int events, int status)
            {
                // eng::log::info("{}: events = {}, status = {}", filename, events, status);
                context_t *ctx = static_cast<context_t*>(handler->data);
                ctx->dirty_files.emplace_back(filename);
                process_dirty_files(*ctx);
            },
            ctx.path.c_str(), 0);

    // Открываем директорию для чтения ее содержимого
    uv_fs_opendir(uv_default_loop(), &ctx.read, ctx.path.c_str(),
            [](uv_fs_t *req)
            {
                context_t *ctx = static_cast<context_t*>(req->data);

                if (req->result < 0)
                {
                    eng::log::error("program-list-model: {}", uv_strerror(req->result));
                    return;
                }

                // Сохраняем указатель на выделенную память
                ctx->dir = static_cast<uv_dir_t*>(req->ptr);
                ctx->dir->dirents = ctx->dirent_storage.data();
                ctx->dir->nentries = ctx->dirent_storage.size();

                uv_fs_req_cleanup(req);

                read_dir_entry(*ctx);
            });
}

void program_list_model::append_file(context_t &ctx)
{
    // Файлов с одинаковым именем в списке может быть два, если у них разная контрольная сумма
    // Ищем файл, у которого установлен бит текущего источника
    auto it = std::ranges::find_if(records_index_map_, [&](auto const &item) {
        return item->filename == ctx.file.front().filename;
    });

    // Если есть, пробуем объединить с имеющимся
    if (it != records_index_map_.end())
    {
        (*it)->syncing = false;

        std::size_t row = std::distance(records_index_map_.begin(), it);

        // Если это обобщенная запись
        if ((*it)->source.count() == 2)
        {
            // И новые данные такие же как в ней
            if ((*it)->crc == ctx.file.front().crc)
            {
                // Оставляем все без изменений
                free_record(ctx);
                dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));

                return;
            }

            // Если отличаются то нам надо разделиться
            (*it)->source.reset(ctx.bit_set);
            dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));
        }
        else
        {
            // Ищем вторую запись
            auto it2 = std::find_if(std::next(it), records_index_map_.end(), [&](auto const &item) {
                return item->filename == ctx.file.front().filename;
            });

            // Если не нашли
            if (it2 == records_index_map_.end())
            {
                // Если новая запись это мы
                if ((*it)->source.test(ctx.bit_set))
                {
                    // Удаляем нас предидущих
                    free_record(records_, *records_index_map_[row]);

                    beginRemoveRows(QModelIndex(), row, row);
                        records_index_map_.erase(std::next(records_index_map_.begin(), row));
                    endRemoveRows();
                }
                else
                {
                    // Если это не мы, сравниваем контрольные суммы
                    if ((*it)->crc == ctx.file.front().crc)
                    {
                        // Если совпадают, объединяем записи
                        (*it)->source.set(ctx.bit_set);

                        free_record(ctx);
                        dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));

                        return;
                    }
                }
            }
            // Если нашли запись с таким-же именем
            else
            {
                if ((*it)->source.test(ctx.bit_set))
                {
                    std::size_t row2 = std::distance(records_index_map_.begin(), it2);

                    // Удаляем нас предидущих
                    free_record(records_, *records_index_map_[row]);

                    beginRemoveRows(QModelIndex(), row, row);
                        records_index_map_.erase(it);
                    endRemoveRows();

                    // Учитываем удаленный
                    row = row2 - 1;
                }
                else
                {
                    std::size_t row2 = std::distance(records_index_map_.begin(), it2);

                    // Удаляем нас предидущих
                    free_record(records_, *records_index_map_[row2]);

                    beginRemoveRows(QModelIndex(), row2, row2);
                        records_index_map_.erase(it2);
                    endRemoveRows();
                }

                it = std::next(records_index_map_.begin(), row);

                // Если новый файл совпадает с имеющимся на другом источнике, объединяем
                if ((*it)->crc == ctx.file.front().crc)
                {
                    (*it)->source.set(ctx.bit_set);
                    free_record(ctx);
                    dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));
                    return;
                }
            }
        }
    }

    // Новые добавляем в начало
    beginInsertRows(QModelIndex(), 0, 0);
        records_.splice(records_.end(), ctx.file, ctx.file.begin());
        records_index_map_.emplace(records_index_map_.begin(), &records_.back());
        records_index_map_.front()->iterator = std::prev(records_.end());
    endInsertRows();
}

void program_list_model::remove_file(context_t &ctx)
{
    // Проверяем, может токой файл уже есть
    auto it = std::ranges::find_if(records_index_map_, [&](auto const &item) {
        return item->filename == ctx.process_file;
    });

    // Файл еще не был добавлен в список
    if (it == records_index_map_.end())
        return;

    std::size_t row = std::distance(records_index_map_.begin(), it);

    // Снимаем с записи о файле свой флаг расположения
    (*it)->source.reset(ctx.bit_set);

    // Если нигде нету, удаляем запись
    if ((*it)->source.none())
    {
        free_record(records_, *records_index_map_[row]);

        beginRemoveRows(QModelIndex(), row, row);
            records_index_map_.erase(std::next(records_index_map_.begin(), row));
        endRemoveRows();

        return;
    }

    // Иначе просто обновляем
    dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));
}

void program_list_model::remove(std::size_t row)
{
    // Помечаем как удаляемый
    records_index_map_[row]->removed = true;
    dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));
}

bool program_list_model::copy_to(context_t &to, context_t const &from, program_record_t &r)
{
    if (r.syncing) return false;
    r.syncing = true;

    bool need_start = to.syncing_files.empty();
    to.syncing_files.emplace_back(from.path / r.filename);
    if (need_start) program_list_model::copy_next_file(to);

    return true;
}

void program_list_model::copy_to_hd(std::size_t row)
{
    if (copy_to(*hd_, *usb_, *records_index_map_[row]))
        dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));
}

void program_list_model::copy_to_usb(std::size_t row)
{
    if (copy_to(*usb_, *hd_, *records_index_map_[row]))
        dataChanged(createIndex(row, 0), createIndex(row, header_.size() - 1));
}

int program_list_model::rowCount(QModelIndex const&) const
{
    return static_cast<int>(records_index_map_.size());
}

int program_list_model::columnCount(QModelIndex const&) const 
{
    return header_.size();
}

QVariant program_list_model::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags program_list_model::flags(QModelIndex const& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    std::size_t const row = index.row();
    program_record_t *r = records_index_map_[row];

    Qt::ItemFlags flags = 0;

    if (!r->removed)
    {
        flags |= Qt::ItemIsSelectable;
        flags |= Qt::ItemIsEnabled;
    }

    return flags;
}

void program_list_model::sort(int column, Qt::SortOrder order)
{
    // 1. Уведомляем систему о начале глобального изменения
    layoutAboutToBeChanged();

    // 2. Сортируем вектор итераторов (сам std::list не трогаем!)
    std::ranges::sort(records_index_map_, [column, order](auto r1, auto r2)
    {
        bool result = false;
        switch(column)
        {
        case 0:
            result = r1->filename < r2->filename;
            break;
        case 1:
            result = r1->last_write_date < r2->last_write_date;
            break;
        }
        return order == Qt::AscendingOrder ? result : !result;
    });

    // 3. Сообщаем Qt, что порядок строк изменился
    layoutChanged();
}

QVariant program_list_model::data(QModelIndex const& index, int role) const
{
    std::size_t const row = index.row();
    std::size_t const col = index.column();

    program_record_t const *r = records_index_map_[row];

    switch(role)
    {
    case Qt::DisplayRole:
        switch(col)
        {
        case 0: {
            auto const &val = r->filename;
            return QString::fromUtf8(val.data(), val.length()); }
        case 1: {
            std::tm *ptm = std::localtime(&r->last_write_date);
            char buffer[32];
            std::strftime(buffer, sizeof(buffer), "%d.%m.%Y", ptm);
            return QString(buffer); }
        }
        break;

    case Qt::DecorationRole:
        switch(col)
        {
        case 2:
            if (!r->source.test(program_record_t::hd_bit)) break;
            return icon_hd_;
        case 3:
            if (!r->source.test(program_record_t::usb_bit)) break;
            return icon_usb_;
        }
        break;

    case Qt::TextAlignmentRole:
        switch(col)
        {
        case 1:
        case 2:
        case 3:
            return Qt::AlignCenter;
        }
        break;

    case Qt::BackgroundRole:
        if (r->removed)
            return QColor("#e56756");
        else if (r->syncing)
            return QColor("#bbf7bb");

        return QVariant();
    }

    return QVariant{};
}

void program_list_model::process_dirty_files(context_t &ctx)
{
    // eng::log::info("{}", __func__);

    // Мы обработали все файлы
    if (ctx.dirty_files.empty())
        return;

    // В данный момент происходит обработка очередного файла
    if (!ctx.process_file.empty())
        return;

    ctx.path /= ctx.dirty_files.back();
    std::swap(ctx.process_file, ctx.dirty_files.back());
    ctx.dirty_files.pop_back();

    uv_fs_stat(uv_default_loop(), &ctx.read, ctx.path.c_str(),
        [](uv_fs_t *req)
        {
            context_t *ctx = static_cast<context_t*>(req->data);

            if (req->result == UV_ENOENT)
            {
                // Возможно файл был удален, уведомляем модель об этом
                ctx->self.remove_file(*ctx);
                ctx->process_file.clear();

                uv_fs_req_cleanup(req);
                process_dirty_files(*ctx);

                return;
            }

            std::size_t fsize = req->statbuf.st_size;
            std::time_t mtime = req->statbuf.st_mtim.tv_sec;

            uv_fs_req_cleanup(req);

            read_file_data(*ctx, fsize, mtime);
        });

    ctx.path.remove_filename();
}

void program_list_model::read_dir_entry(context_t &ctx)
{
    // eng::log::info("{}", __func__);

    uv_fs_readdir(uv_default_loop(), &ctx.read, ctx.dir,
            [](uv_fs_t *req)
            {
                context_t *ctx = static_cast<context_t*>(req->data);

                if (req->result)
                {
                    // eng::log::info("read done: entries = {}", req->result);
                    for (std::size_t i = 0; i < req->result; ++i)
                    {
                        if (ctx->dirent_storage[i].type == UV_DIRENT_FILE)
                        {
                            std::string_view filename = ctx->dirent_storage[i].name;
                            ctx->dirty_files.emplace_back(filename);
                        }
                    }

                    uv_fs_req_cleanup(req);

                    read_dir_entry(*ctx);

                    return;
                }

                uv_fs_req_cleanup(req);

                uv_fs_closedir(uv_default_loop(), req, ctx->dir,
                        [](uv_fs_t *req)
                        {
                            context_t *ctx = static_cast<context_t*>(req->data);

                            ctx->dir = nullptr;

                            uv_fs_req_cleanup(req);
                            process_dirty_files(*ctx);
                        });
            });
}

static constexpr bool analyze_file_data(context_t &ctx)
{
    program_record_t &r = ctx.file.front();

    // Данных недостаточно
    if (r.data.size() <= sizeof(std::uint32_t))
    {
        eng::log::error("{}: Слишком маленький файл", r.filename);
        return false;
    }

    auto span = std::span<std::byte const>{ r.data.data(), r.data.size() };

    r.crc = eng::read_cast<std::uint32_t>(span);
    std::uint32_t fcrc = eng::crc::crc32(span.begin(), span.end());

    // Контрольная сумма не совпадает
    if (r.crc != fcrc)
    {
        eng::log::error("{}: Битый файл", r.filename);
        return false;
    }

    std::uint32_t slen = eng::read_cast<std::uint32_t>(span);
    r.comments = std::string{ reinterpret_cast<char const *>(span.data()), slen };

    r.head_size = slen + sizeof(std::uint32_t) * 2;

    return true;
}

void program_list_model::read_file_data(context_t &ctx, std::size_t fsize, std::time_t mtime)
{
    // eng::log::info("{}", __func__);

    program_record_t &record = alloc_record(ctx);

    record.filename = std::move(ctx.process_file);
    record.data.resize(fsize);

    record.source.reset();
    record.source.set(ctx.bit_set);
    record.removed = false;
    record.syncing = false;

    record.last_write_date = mtime;

    ctx.path /= record.filename;

    int flags = 0;
    int mode = O_RDONLY;

    uv_fs_open(uv_default_loop(), &ctx.read, ctx.path.c_str(), flags, mode,
        [](uv_fs_t *req)
        {
            context_t *ctx = static_cast<context_t*>(req->data);

            ctx->file_handle = req->result;
            uv_fs_req_cleanup(req);

            program_record_t &record = ctx->file.front();
            uv_buf_t buf = uv_buf_init(
                    reinterpret_cast<char*>(record.data.data()),
                    record.data.size()
                );

            uv_fs_read(uv_default_loop(), req, ctx->file_handle, &buf, 1, 0,
                    [](uv_fs_t *req)
                    {
                        context_t *ctx = static_cast<context_t*>(req->data);

                        uv_fs_req_cleanup(req);

                        uv_fs_close(uv_default_loop(), req, ctx->file_handle,
                                [](uv_fs_t *req)
                                {
                                    context_t *ctx = static_cast<context_t*>(req->data);
                                    uv_fs_req_cleanup(req);

                                    if (analyze_file_data(*ctx))
                                        ctx->self.append_file(*ctx);
                                    else
                                        free_record(*ctx);

                                    process_dirty_files(*ctx);
                                });
                    });
        });

    ctx.path.remove_filename();
}

void program_list_model::copy_next_file(context_t &ctx)
{
    // eng::log::info("{}", __func__);

    if (ctx.syncing_files.empty())
        return;

    std::filesystem::path src;
    std::swap(src, ctx.syncing_files.back());
    ctx.syncing_files.pop_back();

    ctx.path /= src.filename();

    int flags = 0;

    uv_fs_copyfile(uv_default_loop(), &ctx.copy, src.c_str(), ctx.path.c_str(), flags,
            [](uv_fs_t *req)
            {
                context_t *ctx = static_cast<context_t*>(req->data);

                if (req->result < 0)
                    eng::log::error("copy_next_file: {}", uv_strerror(req->result));

                uv_fs_req_cleanup(req);
                copy_next_file(*ctx);
            });

    ctx.path.remove_filename();
}

