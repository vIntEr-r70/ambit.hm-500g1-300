#include "ethercat.hpp"
#include "ethercat-slave.hpp"

#include <ecrt.h>
#include <eng/timer.hpp>
#include <eng/utils.hpp>

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <unordered_map>
#include <print>

ec_master_t *master_ = nullptr;

ec_domain_t *domain_ = nullptr;
std::uint8_t *domain_data = nullptr;
static std::vector<unsigned int> domain_data_offset;
static std::vector<ec_pdo_entry_reg_t> domain_regs_;
static std::vector<std::uint8_t> pdo_local_memory;

// static std::function<void(std::string const &, std::string const &, std::string const &)> callback_;

static std::vector<std::tuple<std::string, std::string, std::string>> pdo_bit_set_list;
static std::vector<std::tuple<std::string, std::string, std::string>> pdo_bit_reset_list;
static std::vector<std::tuple<std::string, std::string, std::string>> pdo_value_set_list;

#define NSEC_PER_SEC (1000000000L)
#define TIMESPEC2NS(T) ((uint64_t) (T).tv_sec * NSEC_PER_SEC + (T).tv_nsec)

static inline std::uint32_t w2dw(std::uint16_t index, std::uint8_t subindex)
{
    std::uint32_t key = index;
    key <<= 16;
    return key + subindex;
}

static std::string w2str(std::uint16_t index, std::uint8_t subindex)
{
    return std::format("{:04X}:{:02X}", index, subindex);
}

struct entry_type_t
{
    virtual std::size_t size() const noexcept = 0;
    virtual std::string to_string(std::uint8_t const *) const = 0;
    virtual void set_bit(std::uint8_t *, std::string_view) const = 0;
    virtual void reset_bit(std::uint8_t *, std::string_view) const = 0;
    virtual void set_value(std::uint8_t *, std::string_view) const = 0;
};

template <std::signed_integral T>
struct entry_type_signed_t
    : entry_type_t
{
    std::size_t size() const noexcept override { return sizeof(T) * 8; }

    std::string to_string(std::uint8_t const *ptr) const override
    {
        return std::to_string(*reinterpret_cast<T const *>(ptr));
    }

    void set_bit(std::uint8_t *ptr, std::string_view v) const override
    {
        std::size_t idx;
        std::from_chars(v.data(), v.data() + v.size(), idx);
        *reinterpret_cast<T*>(ptr) |= (1 << idx);
    }

    void reset_bit(std::uint8_t *ptr, std::string_view v) const override
    {
        std::size_t idx;
        std::from_chars(v.data(), v.data() + v.size(), idx);
        *reinterpret_cast<T*>(ptr) &= ~(1 << idx);
    }

    void set_value(std::uint8_t *ptr, std::string_view v) const override
    {
        if (!std::from_chars(v.data(), v.data() + v.size(), *reinterpret_cast<T*>(ptr)))
        {
            std::println("ethercat::from_string: Не удалось получить значение из строки: {}", v);
        }
    }
};

template <std::unsigned_integral T>
struct entry_type_unsigned_t
    : entry_type_t
{
    std::size_t size() const noexcept override { return sizeof(T) * 8; }

    std::string to_string(std::uint8_t const *ptr) const override
    {
        return std::to_string(*reinterpret_cast<T const *>(ptr));
    }

    void set_bit(std::uint8_t *ptr, std::string_view v) const override
    {
        std::size_t idx;
        std::from_chars(v.data(), v.data() + v.size(), idx);
        *reinterpret_cast<T*>(ptr) |= (1 << idx);
    }

    void reset_bit(std::uint8_t *ptr, std::string_view v) const override
    {
        std::size_t idx;
        std::from_chars(v.data(), v.data() + v.size(), idx);
        *reinterpret_cast<T*>(ptr) &= ~(1 << idx);
    }

    void set_value(std::uint8_t *ptr, std::string_view v) const override
    {
        if (!std::from_chars(v.data(), v.data() + v.size(), *reinterpret_cast<T*>(ptr)))
        {
            std::println("ethercat::set_value: Не удалось получить значение из строки: {}", v);
        }
    }
};

template <std::floating_point T>
struct entry_type_floating_t
    : entry_type_t
{
    std::size_t size() const noexcept override { return sizeof(T) * 8; }

    std::string to_string(std::uint8_t const *ptr) const override
    {
        return std::to_string(*reinterpret_cast<T const *>(ptr));
    }

    void set_bit(std::uint8_t *ptr, std::string_view v) const override
    {
        std::println("ethercat::set_bit: Данный регистр это не умеет");
    }

    void reset_bit(std::uint8_t *ptr, std::string_view v) const override
    {
        std::println("ethercat::reset_bit: Данный регистр это не умеет");
    }

    void set_value(std::uint8_t *ptr, std::string_view v) const override
    {
        if (!std::from_chars(v.data(), v.data() + v.size(), *reinterpret_cast<T*>(ptr)))
        {
            std::println("ethercat::from_string: Не удалось получить значение из строки: {}", v);
        }
    }
};

static std::unordered_map<std::string_view, entry_type_t*> const map_of_types {
    { "s8",  new entry_type_signed_t<std::int8_t>()  },
    { "s16", new entry_type_signed_t<std::int16_t>() },
    { "s32", new entry_type_signed_t<std::int32_t>() },

    { "u8",  new entry_type_unsigned_t<std::uint8_t>()  },
    { "u16", new entry_type_unsigned_t<std::uint16_t>() },
    { "u32", new entry_type_unsigned_t<std::uint32_t>() },

    { "f32", new entry_type_floating_t<float>()  },
    { "f64", new entry_type_floating_t<double>() },
};

// struct rw_entry_t
// {
//     std::vector<std::string> bit_set_list;
//     std::vector<std::string> bit_reset_list;
//     std::vector<std::string> value_set_list;
// };

struct pdo_entry_t
{
    pdo_entry_t(std::string key, value_holder_base *type)
        : key(std::move(key)), mem_local(nullptr), mem_hw(nullptr), type(type)
    { }

    std::string key;
    std::uint8_t *mem_local;
    std::uint8_t *mem_hw;
    value_holder_base *type;

    bool upload_value_to_hw()
    {
        return type->upload_value(mem_hw);
    }

    // void set_bit(std::string_view v) {
    //     type->set_bit(mem_hw, v);
    // }
    //
    // void reset_bit(std::string_view v) {
    //     type->reset_bit(mem_hw, v);
    // }
    //
    // void set_value(std::string_view v) {
    //     type->set_value(mem_hw, v);
    // }
};

struct pdo_ro_entry_t
    : public pdo_entry_t
{
    pdo_ro_entry_t(std::string key, value_holder_base_ro *type)
        : pdo_entry_t(std::move(key), type)
    { }
};

struct pdo_rw_entry_t
    : public pdo_entry_t
    // , public rw_entry_t
{
    pdo_rw_entry_t(std::string key, value_holder_base_rw *type)
        : pdo_entry_t(std::move(key), type)
    { }
};

struct sdo_entry_t
{
    sdo_entry_t(std::string key, std::uint16_t index, std::uint8_t subidx, entry_type_t *type)
        : key(std::move(key)), index(index), subidx(subidx), type(type)
    { }

    std::string key;
    std::uint16_t index;
    std::uint8_t subidx;
    entry_type_t *type;

    ec_sdo_request_t *request{ nullptr };
    std::string value;
};

struct sdo_ro_entry_t
    : public sdo_entry_t
{
    sdo_ro_entry_t(std::string key, std::uint16_t index, std::uint8_t subidx, entry_type_t *type)
        : sdo_entry_t(std::move(key), index, subidx, type)
    { }
};

struct sdo_rw_entry_t
    : public sdo_entry_t
    // , public rw_entry_t
{
    sdo_rw_entry_t(std::string key, std::uint16_t index, std::uint8_t subidx, entry_type_t *type)
        : sdo_entry_t(std::move(key), index, subidx, type)
    { }
};

struct slave_t
{
    ethercat_slave *slave;

    ec_slave_config_t *config;

    // Настройки PDO
    std::vector<ec_pdo_entry_info_t> pdo_entry_transmit;
    std::vector<ec_pdo_entry_info_t> pdo_entry_receive;

    std::vector<pdo_ro_entry_t> pdo_ro_entry;
    std::vector<pdo_rw_entry_t> pdo_rw_entry;

    std::vector<sdo_ro_entry_t> sdo_ro_entry;
    std::vector<sdo_rw_entry_t> sdo_rw_entry;
};

static std::vector<slave_t> slaves_;

static void init_slave(slave_t &slave)
{
    slave_info_t cfg = slave.slave->info();
    
    slave.config = ecrt_master_slave_config(master_,
        cfg.target.alias, cfg.target.position, cfg.VendorID, cfg.ProductCode);

    std::println("INIT: {}:{} VendorID = {:#08X} ProductCode = {:#08X}",
            cfg.target.alias, cfg.target.position, cfg.VendorID, cfg.ProductCode);

    if (slave.config == nullptr)
    {
        std::println("ERROR: init_slave: 0");
        return;
    }

    ecrt_slave_config_dc(slave.config, 0x0300, 5000000, 0, 5000000, 0);
    ecrt_slave_config_watchdog(slave.config, 0, 0);

    ec_pdo_info_t pdos[] = {
        { 0x1600, static_cast<unsigned int>(slave.pdo_entry_transmit.size()), slave.pdo_entry_transmit.data() },
        { 0x1a00, static_cast<unsigned int>(slave.pdo_entry_receive.size()), slave.pdo_entry_receive.data() },
    };

    unsigned int n_pdos_output = slave.pdo_entry_transmit.empty() ? 0 : 1;
    unsigned int n_pdos_input = slave.pdo_entry_receive.empty() ? 0 : 1;

    ec_pdo_info_t *dir_output = n_pdos_output ? (pdos + 0) : nullptr;
    ec_pdo_info_t *dir_input = n_pdos_input ? (pdos + 1) : nullptr;

    ec_sync_info_t syncs[] = {
        { 0, EC_DIR_OUTPUT, 0,              nullptr     },
        { 1, EC_DIR_INPUT,  0,              nullptr     },
        { 2, EC_DIR_OUTPUT, n_pdos_output,  dir_output  },
        { 3, EC_DIR_INPUT,  n_pdos_input,   dir_input   },
        { 0xff }
    };

    if (ecrt_slave_config_pdos(slave.config, EC_END, syncs))
    {
        std::println("ERROR: init_slave: 1");
        return;
    }

    // std::ranges::for_each(slave.sdo_ro_entry, [&slave](auto &entry) {
    //     entry.request = ecrt_slave_config_create_sdo_request(slave.config, entry.index, entry.subidx, entry.type->size() / 8);
    // });
    //
    // std::ranges::for_each(slave.sdo_rw_entry, [&slave](auto &entry) {
    //     entry.request = ecrt_slave_config_create_sdo_request(slave.config, entry.index, entry.subidx, entry.type->size() / 8);
    // });
}

static void unknown_slave(std::string_view guid) {
    std::println("ethercat::write_pdo: Неизвестное устройство: {}", guid);
}

static void unknown_slave_entry(std::string_view guid, std::string_view key) {
    std::println("ethercat::write_pdo: Неизвестный параметер: {}:{}", guid, key);
}

template <typename F>
static void find_entry(std::string_view guid, std::string_view key, F fn)
{
    auto islave = std::ranges::find_if(slaves_, [&guid](auto const &slave) {
        return slave.guid == guid;
    });

    if (islave != slaves_.end())
    {
        auto ientry = std::ranges::find_if(islave->pdo_rw_entry, [&key](auto const &entry) {
            return entry.key == key;
        });

        if (ientry != islave->pdo_rw_entry.end())
        {
            fn(*ientry);
        }
        else
        {
            auto ientry = std::ranges::find_if(islave->sdo_rw_entry, [&key](auto const &entry) {
                return entry.key == key;
            });

            if (ientry != islave->sdo_rw_entry.end())
            {
                fn(*ientry);
            }
            else
            {
                unknown_slave_entry(guid, key);
            }
        }
    }
    else
    {
        unknown_slave(guid);
    }
}

template <typename T>
static void apply_slave_pdo_entry_data(std::vector<T> &pdo_entry_list, bool need_initialize)
{
    std::ranges::for_each(pdo_entry_list, [&need_initialize](auto const &entry)
    {
        std::size_t entry_size = entry.type->size();

        // Если данные не изменились, ничего не делаем
        if (!need_initialize && (std::memcmp(entry.mem_local, entry.mem_hw, entry_size) == 0))
            return;
        std::memcpy(entry.mem_local, entry.mem_hw, entry_size);

        entry.type->update(entry.mem_local);
        // std::string value = entry.type->to_string(entry.mem_local);
        // std::println("PDO-READ: {}", entry.key);
        // eng::utils::print_hex(entry.mem_local, entry_size);
        // eng::utils::print_hex(entry.mem_hw, entry_size);
        // callback_(guid, entry.key, std::move(value));
    });
}

template <typename T, typename F>
static void apply_slave_sdo_entry_data(std::vector<T> &sdo_entry_list, F fn)
{
    std::ranges::for_each(sdo_entry_list, [&fn](auto &entry)
    {
        if (entry.request == nullptr)
            return;

        switch (ecrt_sdo_request_state(entry.request))
        {
        case EC_REQUEST_SUCCESS:
            {
                auto value = entry.type->to_string(ecrt_sdo_request_data(entry.request));
                if (value != entry.value)
                {
                    if (entry.value.empty())
                        std::println("SDO-READ: {} = {}", entry.key, value);

                    // entry.value = value;
                    // callback_(guid, entry.key, std::move(value));
                }

                // if (!fn(entry))
                //     ecrt_sdo_request_read(entry.request);
                // else
                //     ecrt_sdo_request_write(entry.request);
            }
            return;

        case EC_REQUEST_ERROR:
            std::println("{:04X}:{:02X} EC_REQUEST_ERROR", entry.index, entry.subidx);
            entry.request = nullptr;
            return;

        case EC_REQUEST_BUSY:
            return;

        case EC_REQUEST_UNUSED:
            ecrt_sdo_request_read(entry.request);
            return;
        }
    });
}

static std::uint8_t *mem_hw_ = nullptr;

static void domain_process()
{
    bool need_initialize = pdo_local_memory.empty();
    if (need_initialize)
    {
        pdo_local_memory.resize(ecrt_domain_size(domain_));

        // Инициализируем указатели на локальную и доменную память для каждого элемента
        std::uint8_t *mem_local = pdo_local_memory.data();

        std::size_t idx = 0;
        std::ranges::for_each(slaves_, [&idx, mem_local](slave_t &slave)
        {
            std::ranges::for_each(slave.pdo_rw_entry, [&idx, mem_local](auto &entry)
            {
                entry.mem_local = mem_local + domain_data_offset[idx];
                entry.mem_hw = mem_hw_ + domain_data_offset[idx];
                idx += 1;
            });

            std::ranges::for_each(slave.pdo_ro_entry, [&idx, mem_local](auto &entry)
            {
                entry.mem_local = mem_local + domain_data_offset[idx];
                entry.mem_hw = mem_hw_ + domain_data_offset[idx];
                idx += 1;
            });
        });
    }

    {
        // std::uint8_t *mem_local = pdo_local_memory.data();
        // if (std::memcmp(mem_local, mem_hw_, pdo_local_memory.size()))
        // {
        //     std::println("DIFF READ DETECT: ");
        //     std::print("LC: ");
        //     for (std::size_t i = 0; i < pdo_local_memory.size(); ++i)
        //         std::print("{:02X} ", mem_local[i]);
        //     std::print("\nHW: ");
        //     for (std::size_t i = 0; i < pdo_local_memory.size(); ++i)
        //         std::print("{:02X} ", mem_hw_[i]);
        //     std::println("");
        // }
    }

    // Обновляем внутреннее состояние системы данными домена
    // Определяем, какие данные изменились и
    // уведомляем об этом наших уважаемых подписчиков
    std::ranges::for_each(slaves_, [&need_initialize](auto &slave)
    {
        apply_slave_pdo_entry_data(slave.pdo_ro_entry, need_initialize);
        apply_slave_pdo_entry_data(slave.pdo_rw_entry, need_initialize);

        std::ranges::for_each(slave.pdo_rw_entry, [&slave](auto &entry)
        {
            if (entry.upload_value_to_hw())
            {
                // std::println("PDO-WRITE: {}", entry.key);
            }
        });
    });
}

namespace ethercat
{

    // bool init(std::function<void(std::string const &, std::string const &, std::string const &)> callback)
    bool init()
    {
#ifdef BUILDROOT

        master_ = ecrt_request_master(0);
        if (!master_)
        {
            std::println("ERROR: ethercat::init: Не удалось получить доступ к мастеру!");
            return false;
        }

        // callback_ = std::move(callback);

        std::ranges::for_each(slaves_, init_slave);

        std::ranges::for_each(slaves_, [](slave_t const &slave)
        {
            std::ranges::for_each(slave.pdo_entry_transmit, [cfg=slave.slave->info()](ec_pdo_entry_info_t entry)
            {
                domain_regs_.emplace_back(
                        cfg.target.alias, cfg.target.position, cfg.VendorID, cfg.ProductCode,
                        entry.index, entry.subindex, nullptr);
            });

            std::ranges::for_each(slave.pdo_entry_receive, [cfg=slave.slave->info()](ec_pdo_entry_info_t entry)
            {
                domain_regs_.emplace_back(
                        cfg.target.alias, cfg.target.position, cfg.VendorID, cfg.ProductCode,
                        entry.index, entry.subindex, nullptr);
            });
        });

        if (!domain_regs_.empty())
        {
            domain_data_offset.reserve(domain_regs_.size());
            std::ranges::for_each(domain_regs_, [](ec_pdo_entry_reg_t &reg)
            {
                domain_data_offset.push_back(0);
                reg.offset = &domain_data_offset.back();
            });

            // Добавляем завершающий экземпляр
            domain_regs_.push_back({});

            domain_ = ecrt_master_create_domain(master_);
            if (ecrt_domain_reg_pdo_entry_list(domain_, domain_regs_.data()))
            {
                std::println("ERROR: ethercat::init: Не удалось зарегистрировать PDO");
                return false;
            }
        }

        if (ecrt_master_activate(master_))
        {
            std::println("ERROR: ethercat::init: Не удалось активировать мастер");
            return false;
        }

        if (domain_)
            mem_hw_ = ecrt_domain_data(domain_);

#endif

        eng::timer::add_ms(2, []
        {
            struct timespec wakeupTime, time;
            clock_gettime(CLOCK_MONOTONIC, &wakeupTime);

            std::uint64_t nsec = wakeupTime.tv_sec * NSEC_PER_SEC + wakeupTime.tv_nsec;

#ifdef BUILDROOT

            ecrt_master_receive(master_);

            if (domain_)
            {
                ecrt_domain_process(domain_);

                ec_domain_state_t domain_state;
                ecrt_domain_state(domain_, &domain_state);

                if (domain_state.wc_state == EC_WC_COMPLETE)
                {
                    domain_process();
#endif
                    std::ranges::for_each(slaves_, [nsec](auto &slave) {
                        slave.slave->update((nsec + 0.0) / NSEC_PER_SEC);
                    });

#ifdef BUILDROOT
                }

                ecrt_domain_queue(domain_);
            }

            ecrt_master_application_time(master_, nsec);
            ecrt_master_sync_slave_clocks(master_);

            ecrt_master_send(master_);
#endif
        });

        // Таймер чтения SDO
        // eng::timer::add_ms(1, []
        // {
        //     std::ranges::for_each(slaves_, [](auto &slave)
        //     {
        //         apply_slave_sdo_entry_data(slave.sdo_ro_entry,
        //                 [](auto const &) {
        //                     return false;
        //                 });
        //
        //         apply_slave_sdo_entry_data(slave.sdo_rw_entry,
        //                 [&slave](auto &entry)
        //                 {
        //                     // std::uint8_t *mem = ecrt_sdo_request_data(entry.request);
        //
        //                     // std::ranges::for_each(entry.bit_set_list, [&](auto &value) {
        //                     //     entry.type->set_bit(mem, value);
        //                     //     std::println("SDO-WRITE-SET-BIT: {} = {}", entry.key, value);
        //                     // });
        //                     //
        //                     // std::ranges::for_each(entry.bit_reset_list, [&](auto &value) {
        //                     //     entry.type->reset_bit(mem, value);
        //                     //     std::println("SDO-WRITE-RESET-BIT: {} = {}", entry.key, value);
        //                     // });
        //                     //
        //                     // std::ranges::for_each(entry.value_set_list, [&](auto &value) {
        //                     //     entry.type->set_value(mem, value);
        //                     //     std::println("SDO-WRITE: {} = {}", entry.key, value);
        //                     // });
        //                     //
        //                     // bool was_changed =
        //                     //     !entry.bit_set_list.empty() ||
        //                     //     !entry.bit_reset_list.empty() ||
        //                     //     !entry.value_set_list.empty();
        //                     //
        //                     // entry.bit_set_list.clear();
        //                     // entry.bit_reset_list.clear();
        //                     // entry.value_set_list.clear();
        //
        //                     // return was_changed;
        //                 });
        //     });
        // });

        return true;
    }

    static slave_t& register_slave(ethercat_slave *slave)
    {
        auto it = std::ranges::find_if(slaves_,
                [slave](auto const &item) {
                    return item.slave == slave;
                });

        if (it != slaves_.end())
            return *it;

        slaves_.emplace_back(slave);

        return slaves_.back();
    }

    void set_bit(std::string_view guid, std::string_view key, std::string_view value)
    {
        // find_entry(guid, key, [&value](rw_entry_t &entry) {
        //     entry.bit_set_list.emplace_back(value);
        // });
    }

    void reset_bit(std::string_view guid, std::string_view key, std::string_view value)
    {
        // find_entry(guid, key, [&value](rw_entry_t &entry) {
        //     entry.bit_reset_list.emplace_back(value);
        // });
    }

    void set_value(std::string_view guid, std::string_view key, std::string_view value)
    {
        // find_entry(guid, key, [&value](rw_entry_t &entry)
        // {
        //     entry.value_set_list.clear();
        //     entry.value_set_list.emplace_back(value);
        // });
    }

    void add_pdo_receive(std::uint16_t index, std::uint8_t subidx, std::string_view type)
    {
        // auto it = map_of_types.find(type);
        // entry_type_t *ptype = (it == map_of_types.end()) ? nullptr : it->second;
        // if (ptype == nullptr)
        //     throw std::runtime_error(std::format("ethercat::add_pdo_transmit: unknown type {}", type));
        //
        // slave_t &slave = slaves_.back();
        // slave.pdo_entry_receive.emplace_back(index, subidx, ptype->size());
        // slave.pdo_ro_entry.emplace_back(w2str(index, subidx), ptype);
    }

    void add_pdo_transmit(std::uint16_t index, std::uint8_t subidx, std::string_view type)
    {
        // auto it = map_of_types.find(type);
        // entry_type_t *ptype = (it == map_of_types.end()) ? nullptr : it->second;
        // if (ptype == nullptr)
        //     throw std::runtime_error(std::format("ethercat::add_pdo_transmit: unknown type {}", type));
        //
        // slave_t &slave = slaves_.back();
        // slave.pdo_entry_transmit.emplace_back(index, subidx, ptype->size());
        // slave.pdo_rw_entry.emplace_back(w2str(index, subidx), ptype);
    }

    void pdo::add(ethercat_slave *slave, std::uint16_t index, std::uint8_t subidx, value_holder_base_ro &vh)
    {
        slave_t &item = register_slave(slave);
        item.pdo_entry_receive.emplace_back(index, subidx, vh.size() * 8);
        item.pdo_ro_entry.emplace_back(w2str(index, subidx), &vh);
    }

    void pdo::add(ethercat_slave *slave, std::uint16_t index, std::uint8_t subidx, value_holder_base_rw &vh)
    {
        slave_t &item = register_slave(slave);
        item.pdo_entry_transmit.emplace_back(index, subidx, vh.size() * 8);
        item.pdo_rw_entry.emplace_back(w2str(index, subidx), &vh);
    }

    void add_ro_sdo(std::uint16_t index, std::uint8_t subidx, std::string_view type)
    {
        auto it = map_of_types.find(type);
        entry_type_t *ptype = (it == map_of_types.end()) ? nullptr : it->second;
        if (ptype == nullptr)
            throw std::runtime_error(std::format("ethercat::add_sdo: unknown type {}", type));

        slave_t &slave = slaves_.back();
        slave.sdo_ro_entry.emplace_back(w2str(index, subidx), index, subidx, ptype);
    }

    void add_rw_sdo(std::uint16_t index, std::uint8_t subidx, std::string_view type)
    {
        auto it = map_of_types.find(type);
        entry_type_t *ptype = (it == map_of_types.end()) ? nullptr : it->second;
        if (ptype == nullptr)
            throw std::runtime_error(std::format("ethercat::add_sdo: unknown type {}", type));

        slave_t &slave = slaves_.back();
        slave.sdo_rw_entry.emplace_back(w2str(index, subidx), index, subidx, ptype);
    }

}


