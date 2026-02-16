#pragma once

#include <QAbstractTableModel>
#include <QIcon>

#include <filesystem>
#include <vector>

struct program_record_t;
struct context_t;

class program_list_model
    : public QAbstractTableModel
{
    void *uv_fs_event_;
    void *uv_fs_;

    void *uv_dir_;
    void *uv_dirent_;

    std::filesystem::path root_path_;
    std::filesystem::path path_;

    std::vector<std::string> dirty_files_;

    std::list<program_record_t> records_;
    std::vector<program_record_t*> records_index_map_;

    std::unique_ptr<context_t> hd_;
    std::unique_ptr<context_t> usb_;

    QStringList header_;

    QIcon icon_hd_;
    QIcon icon_usb_;

public:

    program_list_model();

public:

    void add_local_path(std::filesystem::path);

    void add_usb_path(std::filesystem::path);

public:

    void remove(std::size_t);

    program_record_t const* record(std::size_t) const;

    program_record_t const* record(std::string const &) const;

    void copy_to_hd(std::size_t);

    void copy_to_usb(std::size_t);

private:

    int rowCount(QModelIndex const& = QModelIndex()) const override;

    int columnCount(QModelIndex const& = QModelIndex()) const override;

    QVariant headerData(int, Qt::Orientation, int) const override;

    Qt::ItemFlags flags(QModelIndex const&) const override;

    QVariant data(QModelIndex const&, int = Qt::DisplayRole) const override;

    void sort(int, Qt::SortOrder) override;

private:

    void start_monitoring_directory(context_t &);

    void append_file(context_t &);

    void remove_file(context_t &);

    void remove_record(std::size_t);

    bool copy_to(context_t &, context_t const &, program_record_t &);

private:

    static void read_dir_entry(context_t &);

    static void process_dirty_files(context_t &);

    static void read_file_data(context_t &, std::size_t, std::time_t);

    static void copy_next_file(context_t &);

// public:
//
//     static bool load_program_comments(QString, QString &) noexcept;
//
// public:
//
//     bool save_to_file() const noexcept;
//
//     bool load_from_local_file(QString) noexcept;
//
//     bool load_from_usb_file(QString const&) noexcept;
//
// public:
//
//     program const& prog() const noexcept { return program_; }
//
//     void set_edited_row(std::size_t row) noexcept {
//         edited_row_ = row;
//     }
//
//     void clear_current_row();
//
//     void set_current_row(std::size_t) noexcept;
//
//     void set_current_phase(std::size_t) noexcept;
//
//     void change_sprayer(std::size_t, std::size_t) noexcept;
//
//     void change_main(std::size_t, std::size_t, std::size_t, double) noexcept;
//
//     void change_pause(std::size_t, std::uint64_t) noexcept;
//
//     void change_loop(std::size_t, std::size_t, std::size_t) noexcept;
//
//     void change_fc(std::size_t, float, float, float) noexcept;
//
//     void change_center(std::size_t, centering_type, float) noexcept;
//
//     void add_op(program::op_type) noexcept;
//
//     void add_main_op(bool) noexcept;
//
//     void remove_op() noexcept;
//
//     std::size_t last_insert_row() const noexcept;
//
//     std::string get_base64_program() const noexcept;
//
//     QString const& name() const noexcept { return name_; } 
//
//     QString const& comments() const noexcept { return comments_; }
//
//     void set_name(QString const& name) noexcept { name_ = name; }
//
//     void set_comments(QString const& comments) noexcept { comments_ = comments; }
//
//     void reset() noexcept;
//
//     bool empty() const noexcept;
//
// private:
//
//     bool data_main_op_equal(std::size_t, std::size_t) const;
//
//     QString data_main_op_text(std::size_t, std::size_t) const;
//
// private:
//
//     bool load_from_file(QString const&) noexcept;
//
// private:
//
//     program program_;
//
//     QString name_;
//     QString comments_;
//
//     std::size_t edited_row_{ 0 };
//     std::optional<std::size_t> current_row_;
//
//     std::filesystem::path path_;
};


