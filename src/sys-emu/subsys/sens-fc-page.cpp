#include "sens-fc-page.hpp"
#include "widgets/sens-value-set.hpp"
#include "listeners/syslink.hpp"
#include "listeners/devices/PR205-A2.hpp"
#include "listeners/devices/PR200.hpp"
#include "listeners/devices/PR205-B2.hpp"
#include "listeners/devices/PR205-B6.hpp"
#include "listeners/devices/PR205-B10.hpp"

#include <QGridLayout>

sens_fc_page::sens_fc_page(QWidget *parent)
    : QWidget(parent)
{
    QFont f(font());
    f.setPointSize(12);
    setFont(f);

    QGridLayout *gL = new QGridLayout(this);
    {
        for (std::size_t i = 0; i < 19; ++i)
        {
            std::string key = std::format("FC{}", i + 1);
            auto item = new sens_value_set(this, QString::fromStdString(key));
            gL->addWidget(item, i / 10, i % 10);
            connect(item, &sens_value_set::change_value, [this, i](int value) {
                update_register_value(i, value); 
            });
        }
    }
}

template <typename T>
static void dev_set(std::size_t idx, std::int16_t value)
{
    syslink::device<T>().fc_set(idx, value);
}

void sens_fc_page::update_register_value(std::size_t idx, std::int16_t value)
{
    static std::unordered_map<std::size_t, std::pair<void(*)(std::size_t idx, std::int16_t), std::size_t>> const map {
        { 1,  { dev_set<devices::PR205_A2>,  8 } },
        { 2,  { dev_set<devices::PR205_A2>,  7 } },
        { 3,  { dev_set<devices::PR205_A2>,  6 } },
        { 4,  { dev_set<devices::PR205_A2>,  5 } },
        { 5,  { dev_set<devices::PR205_A2>,  4 } },
        { 6,  { dev_set<devices::PR205_A2>,  2 } },
        { 7,  { dev_set<devices::PR205_A2>,  1 } },
        { 8,  { dev_set<devices::PR205_A2>,  3 } },
        { 9,  { dev_set<devices::PR205_B2>,  4 } },
        { 10, { dev_set<devices::PR205_B2>,  2 } },
        { 11, { dev_set<devices::PR205_B6>,  4 } },
        { 12, { dev_set<devices::PR205_B6>,  2 } },
        { 13, { dev_set<devices::PR205_B10>, 4 } },
        { 14, { dev_set<devices::PR205_B10>, 2 } },
        { 15, { dev_set<devices::PR205_B2>,  3 } },
        { 16, { dev_set<devices::PR205_B6>,  3 } },
        { 17, { dev_set<devices::PR205_B10>, 3 } },
        { 18, { dev_set<devices::PR200>,     1 } },
        { 19, { dev_set<devices::PR200>,     2 } },
    };

    auto const &item = map.at(idx + 1);
    (*item.first)(item.second - 1, value);
}
