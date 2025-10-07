#include "MnemonicWidget.h"
#include "MnemonicCommon.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>

#include <aem/toml.hpp>
#include <aem/log.h>
#include <aem/environment.h>
#include <global.h>

#include <filesystem>

MnemonicWidget::MnemonicWidget(QWidget *parent, MnemonicImages const& images) noexcept
    : QWidget(parent)
    , images_(images)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    std::filesystem::path path(aem::getenv("LIAEM_RO_PATH", "."));
    
    auto fname = path / "mimic.toml";
    try
    {
        toml::table const tbl = toml::parse_file(fname.string());
        
        auto const &pages = tbl["pages"];
        if (toml::array const *arr = pages.as_array())       
        {
            arr->for_each([this, &path](toml::table const &item)
            {
                std::size_t page_idx = background_.size();
            
                add_page_sensors(item, "pressure", sensor::pressure, page_idx);
                add_page_sensors(item, "temperature", sensor::temperature, page_idx);
                add_page_sensors(item, "inflow", sensor::inflow, page_idx);
                add_page_sensors(item, "flags", sensor::sbit, page_idx);
                
                if (auto const showers = item["showers"].as_table())
                {
                    showers->for_each([this](toml::key const& key, toml::array const &mf) 
                    {
                        int x = mf[0].value_or(0); int y = mf[1].value_or(0);
                        std::size_t idx = 0;
                        std::from_chars(key.str().data(), key.str().data() + key.str().size(), idx);
                        if (idx >= showers_.size()) showers_.resize(idx + 1);
                        showers_[idx] = { QPoint(x, y), 0 };
                    });
                }
                
                if (auto const valves = item["valves"].as_table())
                {
                    valves->for_each([this, page_idx](toml::key const& key, toml::table const &item) 
                    {
                        auto it = valves_.emplace(key.str(), valve_t{ QPoint(0, 0), false, { }, page_idx, placement::bottom });
                        auto &valve = it.first->second;
                        
                        if (auto const mf = item["pos"].as_array())
                        {
                            int x = (*mf)[0].value_or(0); int y = (*mf)[1].value_or(0);
                            valve.pos = QPoint(x, y);
                        }
                        
                        if (auto const showers = item["showers"].as_array())
                        {
                            for (std::size_t i = 0; i < showers->size(); ++i)
                                valve.showers.push_back((*showers)[i].value_or(0));
                        }

                        auto value = item["title-placement"].value_or(std::string_view{});
                        valve.title_placement = convert_string_to_placement(value, valve.title_placement);
                    });
                }
                
                if (auto const slevels = item["slevels"].as_table())
                {
                    slevels->for_each([this, page_idx](toml::key const& key, toml::array const &mf) 
                    {
                        int x = mf[0].value_or(0); int y = mf[1].value_or(0);
                        slevels_[std::string(key.str())] = { QPoint(x, y), false, page_idx, placement::top };
                    });
                }
                
                if (auto const heaters = item["heaters"].as_table())
                {
                    heaters->for_each([this, page_idx](toml::key const& key, toml::table const &item) 
                    {
                        auto it = heaters_.emplace(key.str(), heater_t{ QPoint(0, 0), false, page_idx, placement::bottom });
                        auto &heater = it.first->second;
                        
                        if (toml::array const *mf = item["pos"].as_array())
                        {
                            int x = (*mf)[0].value_or(0); int y = (*mf)[1].value_or(0);
                            heater.pos = QPoint(x, y);
                        }
                        
                        auto value = item["title-placement"].value_or(std::string_view{});
                        heater.title_placement = convert_string_to_placement(value, heater.title_placement);
                    });
                }
                
                if (auto const pumps = item["pumps"].as_table())
                {
                    pumps->for_each([this, page_idx](toml::key const& key, toml::table const &item) 
                    {
                        auto it = pumps_.emplace(key.str(), pump_t{ QPoint(0, 0), false, page_idx, placement::top, false });
                        auto &pump = it.first->second;
                        
                        if (toml::array const *mf = item["pos"].as_array())
                        {
                            int x = (*mf)[0].value_or(0); int y = (*mf)[1].value_or(0);
                            pump.pos = QPoint(x, y);
                        }
                        
                        auto value = item["title-placement"].value_or(std::string_view{});
                        pump.title_placement = convert_string_to_placement(value, pump.title_placement);

                        pump.mirror_x = item["mirror-x"].value_or(false);
                    });
                }
                
                std::filesystem::path png(path); 
                png /= item["background"].value_or("");
                
                background_.push_back({});
                if (!background_.back().load(png.c_str()))
                    aem::log::error("Не удалось загрузить изображение мнемосхемы: {}", png.c_str());
            });
        }
    }
    catch(std::exception const& err) 
    {
        aem::log::error("MnemonicWidget[{}]: Не удалось загрузить файл описания ошибок: {}", fname.string(), err.what());
    }
    
    font_value_ = font();
    font_value_.setPointSize(10);
    font_value_.setBold(true);

    font_units_ = font();
    font_units_.setPointSize(7);

    font_sbits_ = font();
    font_sbits_.setPointSize(10);

    draw_[sensor::pressure]     = &MnemonicWidget::h_draw_pressure;
    draw_[sensor::temperature]  = &MnemonicWidget::h_draw_temperature;
    draw_[sensor::inflow]       = &MnemonicWidget::h_draw_inflow;
    draw_[sensor::sbit]         = &MnemonicWidget::h_draw_sbit;

    global::rpc().call("get", { "locker", "list", { } })
        .done([this](nlohmann::json const &result) {
            initialize_names(result);
        });
    
    startTimer(500);
}

void MnemonicWidget::add_page_sensors(toml::table const &page, std::string const &name, sensor type, std::size_t page_id)
{
    if (auto const tbl = page[name].as_table())
    {
        tbl->for_each([&](toml::key const& key, toml::array const &mf) 
        {
            int x = mf[0].value_or(0); int y = mf[1].value_or(0);
            
            std::string skey(key.str());
            sensors_[skey] = { 
                type, 
                QPoint(x, y), 
                we::locker::item_status::normal, 
                page_id, QString::fromStdString(skey) };
        });
    }
}

void MnemonicWidget::switch_to(std::size_t page_id)
{
    page_id = std::min(page_id, background_.size());
    if (page_id_ == page_id)
        return;
    page_id_ = page_id;
    update();
}

void MnemonicWidget::update_sensor_value(std::string const& key, float v) noexcept
{
    auto it = sensors_.find(key);
    if (it == sensors_.end())
        return;
    values_[key] = QString::number(v, 'f', 1);
    QWidget::update();
}

void MnemonicWidget::update_sensor_state(std::string const& key, we::locker::item_status s) noexcept
{
    auto it = sensors_.find(key);
    if (it == sensors_.end())
    {
        aem::log::warn("MnemonicWidget::update_sensor_state: Датчик не найден: {}", key);
        return;
    }

    if (it->second.status == s)
        return;

    it->second.status = s;
    QWidget::update();
}

void MnemonicWidget::update_valve_state(std::string const& key, bool opened) noexcept
{
    auto it = valves_.find(key);
    if (it == valves_.end())
    {
        aem::log::warn("MnemonicWidget::update_valve_state: Вентиль не найден: {}", key);
        return;
    }

    auto &v = it->second;

    if (v.opened == opened)
        return;
    v.opened = opened;

    std::for_each(v.showers.begin(), v.showers.end(), 
        [this, opened](std::size_t id) { showers_[id].simg = opened ? 1 : 0; });

    QWidget::update();
}

void MnemonicWidget::update_slevel_state(std::string const& key, bool detected) noexcept
{
    auto it = slevels_.find(key);
    if (it == slevels_.end())
    {
        aem::log::warn("MnemonicWidget::update_slevel_state: Датчик уровня не найден: {}", key);
        return;
    }

    auto &v = it->second;

    if (v.detected == detected)
        return;
    v.detected = detected;

    QWidget::update();
}

void MnemonicWidget::update_pump_state(std::string const& key, bool powered) noexcept
{
    auto it = pumps_.find(key);
    if (it == pumps_.end())
    {
        aem::log::warn("MnemonicWidget: Насос не найден: {}", key);
        return;
    }

    auto &v = it->second;

    if (v.powered == powered)
        return;
    v.powered = powered;

    QWidget::update();
}

void MnemonicWidget::update_heater_state(std::string const &key, bool powered) noexcept
{
    auto it = heaters_.find(key);
    if (it == heaters_.end())
    {
        aem::log::warn("MnemonicWidget: Нагреватель не найден: {}", key);
        return;
    }

    auto &v = it->second;

    if (v.powered == powered)
        return;
    v.powered = powered;

    QWidget::update();
}

void MnemonicWidget::paintEvent(QPaintEvent*) noexcept
{
    QPainter gdc(this);

    int x = 0;
    int y = 0;
            
    if (page_id_ != background_.size() && !background_[page_id_].isNull())
    {
        auto const &bg = background_[page_id_];
        
        x += (width() - bg.width()) / 2;
        y += (height() - bg.height()) / 2;
        gdc.setRenderHint(QPainter::Antialiasing);
        gdc.drawPixmap(x, y, bg);
    }
    
    for (auto const& s : sensors_)
    {
        if (s.second.page != page_id_)
            continue;
        
        auto it = draw_.find(s.second.type);
        if (it == draw_.end())
            continue;

        gdc.save();
        int hw = isize_.width() / 2;
        int hh = isize_.height() / 2;
        gdc.translate(x + s.second.pos.x() - hw, y + s.second.pos.y() - hh);
        std::invoke(it->second, this, s.first, gdc, s.second);
        gdc.restore();
    }

    for (auto const& pair : valves_)
    {
        if (pair.second.page != page_id_)
            continue;
        draw_valve(gdc, pair.first, pair.second, x, y);
    }
    
    for (auto const& pair : slevels_)
    {
        if (pair.second.page != page_id_)
            continue;
        draw_slevel(gdc, pair.first, pair.second, x, y);
    }
    
    for (auto const& pair : heaters_)
    {
        if (pair.second.page != page_id_)
            continue;
        draw_heater(gdc, pair.first, pair.second, x, y);
    }
    
    for (auto const& pair : pumps_)
    {
        if (pair.second.page != page_id_)
            continue;
        draw_pump(gdc, pair.first, pair.second, x, y);
    }
}

void MnemonicWidget::timerEvent(QTimerEvent*) noexcept
{
    std::for_each(showers_.begin(), showers_.end(), [](auto &sw)
    {
        if (sw.simg == 0) return;
        sw.simg = (sw.simg == 1) ? 2 : 1;
    });
    QWidget::update();
}

void MnemonicWidget::h_draw_pressure(std::string const& key, QPainter& gdc, sensor_t const &sensor) noexcept
{
    auto const& sens = MnemonicInfo::pressure();
    draw_sensor(key, sens.units, gdc, sens.color, sensor);
}

void MnemonicWidget::h_draw_temperature(std::string const& key, QPainter& gdc, sensor_t const &sensor) noexcept
{
    auto const& sens = MnemonicInfo::temperature();
    draw_sensor(key, sens.units, gdc, sens.color, sensor);
}

void MnemonicWidget::h_draw_inflow(std::string const& key, QPainter& gdc, sensor_t const &sensor) noexcept
{
    auto const& sens = MnemonicInfo::inflow();
    draw_sensor(key, sens.units, gdc, sens.color, sensor);
}

void MnemonicWidget::h_draw_sbit(std::string const& key, QPainter& gdc, sensor_t const &sensor) noexcept
{
    gdc.setFont(font_sbits_);
    gdc.setPen(QPen(sensor.status == we::locker::item_status::critical ? Qt::red : QColor("#B9B9B9")));
    gdc.drawText(0, 0, 200, 30, Qt::AlignCenter, sensor.name);
}

void MnemonicWidget::show_info(bool flag)
{
    show_info_ = flag;
    update();
}

void MnemonicWidget::draw_sensor(std::string const& key, QString const& units, QPainter& gdc, QColor const& color, sensor_t const &sensor) noexcept
{
    unsigned const w = isize_.width();
    unsigned const h = isize_.height();

    QPainterPath path;
    path.addRoundedRect(0, 0, w, h, 10, 10);
    gdc.fillPath(path, color);

    if (sensor.status != we::locker::item_status::normal)
    {
        QPen pen;
        pen.setWidth(5);
        pen.setColor(sensor.status == we::locker::item_status::critical ? Qt::red : Qt::yellow);
        gdc.strokePath(path, pen);
    }

    int hh = h / 2.5;

    gdc.setPen(Qt::black);
    gdc.setFont(font_units_);

    // Выводим единицы измерения либо идентификатор
    if (show_info_)
        gdc.drawText(0, h - hh, w, hh, Qt::AlignCenter, sensor.name);
    else
        gdc.drawText(0, h - hh, w, hh, Qt::AlignCenter, units);

    auto it = values_.find(key);
    if (it == values_.end())
        return;

    gdc.setPen(Qt::white);
    gdc.setFont(font_value_);

    // Выводим измеренное значение
    gdc.drawText(0, 0, w, h - hh, Qt::AlignCenter, it->second);
}

void MnemonicWidget::draw_valve(QPainter& gdc, std::string const& key, valve_t const& v, int x, int y) noexcept
{
    gdc.save();
    gdc.translate(x + v.pos.x(), y + v.pos.y());

    int iid = v.opened ? MnemonicImages::valve_opened : MnemonicImages::valve_closed;
    gdc.drawPixmap(0, 0, images_[iid]);

    if (show_info_)
        draw_title(gdc, key, iid, v.title_placement);
    
    gdc.restore();

    std::for_each(v.showers.begin(), v.showers.end(), 
        [&](std::size_t id) { draw_shower(gdc, showers_[id], x, y); });
}

void MnemonicWidget::draw_shower(QPainter& gdc, shower_t const& sw, int x, int y) noexcept
{
    gdc.save();
    gdc.translate(x + sw.pos.x(), y + sw.pos.y());

    static std::array<int, 3> states{ 
        MnemonicImages::shower_off, MnemonicImages::shower_half, MnemonicImages::shower_max };

    std::size_t state = std::min(states.size(), sw.simg);
    gdc.drawPixmap(0, 0, images_[states[state]]);

    gdc.restore();
}

void MnemonicWidget::draw_slevel(QPainter& gdc, std::string const& key, slevel_t const& v, int x, int y) noexcept
{
    gdc.save();
    gdc.translate(x + v.pos.x(), y + v.pos.y());

    int iid = v.detected ? MnemonicImages::slevel_on: MnemonicImages::slevel_off;
    gdc.drawPixmap(0, 0, images_[iid]);

    if (show_info_)
        draw_title(gdc, key, iid, v.title_placement);

    gdc.restore();
}

void MnemonicWidget::draw_heater(QPainter& gdc, std::string const& key, heater_t const& v, int x, int y) noexcept
{
    gdc.save();
    gdc.translate(x + v.pos.x(), y + v.pos.y());

    int iid = v.powered ? MnemonicImages::heater_on: MnemonicImages::heater_off;
    gdc.drawPixmap(0, 0, images_[iid]);

    if (show_info_)
        draw_title(gdc, key, iid, v.title_placement);

    gdc.restore();
}

void MnemonicWidget::draw_pump(QPainter& gdc, std::string const& key, pump_t const& v, int x, int y) noexcept
{
    gdc.save();
    gdc.translate(x + v.pos.x(), y + v.pos.y());

    int iid = v.powered ? MnemonicImages::pump_on: MnemonicImages::pump_off;
    
    if (show_info_)
        draw_title(gdc, key, iid, v.title_placement);

    int new_x = 0;
    if (v.mirror_x)
    {
        gdc.scale(-1, 1);
        new_x -= images_[iid].width();
    }
    gdc.drawPixmap(new_x, 0, images_[iid]);

    gdc.restore();
}

void MnemonicWidget::draw_title(QPainter &gdc, std::string const &title, int iid, placement title_placement)
{
    int iw = images_[iid].width();
    int ih = images_[iid].height();
    
    gdc.setPen(Qt::black);
    gdc.setFont(font_units_);

    QFontMetrics const metrics(font_units_);
    int tw = metrics.horizontalAdvance(title.c_str());
    int th = metrics.height();

    int tx = 0;
    int ty = 0;
    
    switch (title_placement)
    {
    case placement::top:
    case placement::bottom:
        tx = (iw - tw) / 2;
        break;
    case placement::left:
        tx = -tw - 3;
        break;
    case placement::right:
        tx = 3;
        break;
    };
    
    switch (title_placement)
    {
    case placement::left:
    case placement::right:
        ty = (ih - th) / 2;
        break;
    case placement::top:
        ty = -th;
        break;
    case placement::bottom:
        ty = ih;
        break;
    };
    
    gdc.drawText(tx, ty, tw, th, Qt::AlignCenter, title.c_str());
    // gdc.fillRect(QRect(tx, ty, tw, th), QColor("#80808080"));
}

void MnemonicWidget::initialize_names(nlohmann::json const &cfg)
{
    try
    {
        auto const flags = cfg.find("flags"); 
        if (flags != cfg.end())
        {
            for (auto const &flag : flags->items()) 
            {
                auto it = sensors_.find(flag.key());
                if (it == sensors_.end())
                    continue;
                
                auto it2 = flag.value().find("name");
                if (it2 == flag.value().end())
                    continue;
                
                it->second.name = QString::fromStdString(it2->get_ref<std::string const &>());
            }
        }
        
        auto const intervals = cfg.find("intervals"); 
        if (intervals != cfg.end())
        {
            for (auto const &sensor : cfg["intervals"].items()) 
            {
                auto it = sensors_.find(sensor.key());
                if (it == sensors_.end())
                    continue;
                
                auto it2 = sensor.value().find("name");
                if (it2 == sensor.value().end())
                    continue;
                
                it->second.name = QString::fromStdString(it2->get_ref<std::string const &>());
            }
        }
    }
    catch(std::exception const &e)
    { }
}

MnemonicWidget::placement MnemonicWidget::convert_string_to_placement(std::string_view value, placement def) const
{
    if (value == "top")
        return placement::top;
    else if (value == "left")
        return placement::left;
    else if (value == "right")
        return placement::right;
    else if (value == "bottom")
        return placement::bottom;

    return def; 
}
