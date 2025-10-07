#pragma once

#include <we/locker.h>

#include <QWidget>
#include <QPixmap>

#include <vector>

class QPixmap;
class QPainter;
class QPaintEvent;
class ValueSetBool;
struct MnemonicImages;

class MnemonicWidget final
    : public QWidget
{
    enum class sensor : int
    {
        pressure,
        temperature,
        inflow,
        sbit 
    };

    enum class placement
    {
        top,
        left,
        bottom,
        right
    };

    struct sensor_t
    {
        sensor type;
        QPoint pos;
        we::locker::item_status status;
        std::size_t page;
        QString name;
    };

    struct valve_t
    {
        QPoint pos;
        bool opened;
        std::vector<std::size_t> showers;
        std::size_t page;
        placement title_placement;
    };
    
    struct shower_t
    {
        QPoint pos;
        std::size_t simg; 
    };
    
    struct slevel_t
    {
        QPoint pos;
        bool detected;
        std::size_t page;
        placement title_placement;
    };
    
    struct heater_t
    {
        QPoint pos;
        bool powered;
        std::size_t page;
        placement title_placement;
    };
    
    struct pump_t
    {
        QPoint pos;
        bool powered;
        std::size_t page;
        placement title_placement;
        bool mirror_x;
    };

    typedef void(MnemonicWidget::*draw_handler_t)(std::string const&, QPainter&, sensor_t const &);

public:

    MnemonicWidget(QWidget*, MnemonicImages const&) noexcept;

public:

    void show_info(bool);

    void update_sensor_value(std::string const&, float) noexcept;

    void update_sensor_state(std::string const&, we::locker::item_status) noexcept;

    void update_valve_state(std::string const&, bool) noexcept;
    
    void update_slevel_state(std::string const&, bool) noexcept;
    
    void update_pump_state(std::string const&, bool) noexcept;
    
    void update_heater_state(std::string const&, bool) noexcept;

    std::size_t pages() const noexcept { return background_.size(); }

    void switch_to(std::size_t);

private:

    void paintEvent(QPaintEvent*) noexcept override final;

    void timerEvent(QTimerEvent*) noexcept override final;

private:

    void h_draw_pressure(std::string const&, QPainter&, sensor_t const &) noexcept;

    void h_draw_temperature(std::string const&, QPainter&, sensor_t const &) noexcept;

    void h_draw_inflow(std::string const&, QPainter&, sensor_t const &) noexcept;

    void h_draw_sbit(std::string const&, QPainter&, sensor_t const &) noexcept;

    void draw_sensor(std::string const&, QString const&, QPainter&, QColor const&, sensor_t const &) noexcept;

private:

    void draw_valve(QPainter&, std::string const&, valve_t const&, int, int) noexcept;
    
    void draw_shower(QPainter&, shower_t const&, int, int) noexcept;
    
    void draw_slevel(QPainter&, std::string const&, slevel_t const&, int, int) noexcept;
    
    void draw_heater(QPainter&, std::string const&, heater_t const&, int, int) noexcept;
    
    void draw_pump(QPainter&, std::string const&, pump_t const&, int, int) noexcept;

    void initialize_names(nlohmann::json const &);

    void add_page_sensors(toml::table const &, std::string const &, sensor, std::size_t);
    
    void draw_title(QPainter &, std::string const &, int, placement);

    placement convert_string_to_placement(std::string_view, placement) const;
    
private:

    std::vector<QPixmap> background_;
    std::size_t page_id_{ 0 };
    
    QFont font_value_;
    QFont font_units_;
    QFont font_sbits_;

    QSize const isize_{ 41, 41 };

    std::map<std::string, sensor_t> sensors_;
    std::map<sensor, draw_handler_t> draw_;
    std::map<std::string, QString> values_;

    std::map<std::string, valve_t> valves_;
    std::vector<shower_t> showers_;
    
    std::map<std::string, slevel_t> slevels_;
    std::map<std::string, heater_t> heaters_;
    std::map<std::string, pump_t> pumps_;

    MnemonicImages const& images_;

    bool show_info_{ false };
};

