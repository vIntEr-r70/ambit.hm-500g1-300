#include "sens-dp-page.hpp"
#include "widgets/sens-value-set.hpp"

#include "listeners/syslink.hpp"
#include "listeners/devices/PR205-A1.hpp"
#include "listeners/devices/PR205-A2.hpp"
#include "listeners/devices/PR205-A14.hpp"

#include <QGridLayout>

sens_dp_page::sens_dp_page(QWidget *parent)
    : QWidget(parent)
{
    QFont f(font());
    f.setPointSize(12);
    setFont(f);

    QGridLayout *gL = new QGridLayout(this);
    {
        for (std::size_t i = 0; i < 4; ++i)
        {
            std::string key = std::format("DP{}", i + 1);
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
    syslink::device<T>().dp_set(idx, value);
}

void sens_dp_page::update_register_value(std::size_t idx, std::int16_t value)
{
    static std::unordered_map<std::size_t, std::pair<void(*)(std::size_t idx, std::int16_t), std::size_t>> const map {
        { 1,  { dev_set<devices::PR205_A1>,  1 } },
        { 2,  { dev_set<devices::PR205_A1>,  2 } },
        { 3,  { dev_set<devices::PR205_A2>,  1 } },
        { 4,  { dev_set<devices::PR205_A14>, 1 } },
    };

    auto const &item = map.at(idx + 1);
    (*item.first)(item.second - 1, value);
}
