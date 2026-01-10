#include "ProgramModelHeader.h"
#include "common/program.h"

#include <QTableWidget>

std::pair<std::size_t, std::size_t> ProgramModelHeader::cell(program const &p, std::size_t column) noexcept
{
    if (column == 0)
        return { Num, 0 };

    // Приводим индекс с программному
    std::size_t idx = column - 1;

    auto idx_check = [](std::size_t &idx, std::size_t max)
    {
        if (idx < max) return true;
        idx -= max; return false;
    };

    // Сначала ПЧ
    if (idx_check(idx, p.fc_count * 2))
    {
        if (idx % 2)
            return { FcI, idx / 2 };
        else
            return { FcP, idx / 2 };
    }

    // Теперь спрейер
    if (idx_check(idx, p.sprayer_count))
        return { Sprayer, idx };

    // Теперь вращение
    if (idx_check(idx, p.s_axis.size()))
        return { Spin, idx };

    // Теперь позиционирование
    if (!p.t_axis.empty())
    {
        if (idx_check(idx, p.t_axis.size()))
            return { TargetPos, idx };

        return { TargetSpeed, idx };
    }

    return { 0, 0 };
}

std::size_t ProgramModelHeader::column_count(program const& p) noexcept
{
    return 1 + (p.fc_count * 2) + p.sprayer_count + p.s_axis.size() + (p.t_axis.size() + 2);
}

void ProgramModelHeader::create_header(program const& p, QTableWidget& header, int width) noexcept
{
    header.setWordWrap(true);
    header.setColumnCount(column_count(p));
    header.setRowCount(3);

    auto new_item = [](QString const& title)
    {
        auto item = new QTableWidgetItem(title);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    std::size_t idx = 0;

    // Индекс
    header.setSpan(0, idx, 3, 1);
    header.setItem(0, idx, new_item("№"));
    header.setColumnWidth(idx, 50);
    width -= 50;
    idx += 1;

    // Сначала ПЧ
    for (std::size_t i = 0; i < p.fc_count; ++i)
    {
        header.setSpan(0, idx, 1, 2);
        header.setItem(0, idx, new_item(QString("ПЧ №%1").arg(i + 1)));
        header.setSpan(1, idx + 0, 2, 1);
        header.setItem(1, idx + 0, new_item("Мощность\nкВт"));
        header.setSpan(1, idx + 1, 2, 1);
        header.setItem(1, idx + 1, new_item("Ток\nА"));
        header.setColumnWidth(idx + 0, 85);
        width -= 85;
        header.setColumnWidth(idx + 1, 75);
        width -= 75;
        idx += 2;
    }

    // Теперь спрейер
    if (p.sprayer_count)
    {
        header.setSpan(0, idx, 1, p.sprayer_count);
        header.setItem(0, idx, new_item("Спрейер"));
        for (std::size_t i = 0; i < p.sprayer_count; ++i)
        {
            header.setSpan(1, idx + i, 2, 1);
            header.setItem(1, idx + i, new_item(QString("№%1").arg(i + 1)));
            header.setColumnWidth(idx + i, 50);
            width -= 50;
        }
        idx += p.sprayer_count;
    }

    // Теперь вращение
    if (!p.s_axis.empty())
    {
        header.setSpan(0, idx, 1, p.s_axis.size());
        header.setItem(0, idx, new_item("Вращение"));
        header.setSpan(1, idx, 1, p.s_axis.size());
        header.setItem(1, idx, new_item("Скорость, об/мин"));
        for (std::size_t i = 0; i < p.s_axis.size(); ++i)
        {
            header.setItem(2, idx + i, new_item(QString("%1").arg(p.s_axis[i])));
            header.setColumnWidth(idx + i, 70);
            width -= 70;
        }
        idx += p.s_axis.size();
    }

    // Теперь позиционирование
    if (!p.t_axis.empty())
    {
        header.setSpan(0, idx, 1, p.t_axis.size() + 2);
        header.setItem(0, idx, new_item("Позиционирование"));

        for (std::size_t i = 0; i < p.t_axis.size(); ++i)
        {
            char axis = p.t_axis[i];
            bool spin = std::find(p.s_axis.begin(), p.s_axis.end(), axis) != p.s_axis.end();
            header.setSpan(1, idx + i, 2, 1);
            header.setItem(1, idx + i, new_item(QString("%1\n%2").arg(axis).arg(spin ? "град" : "мм")));
            header.setColumnWidth(idx + i, 68);
            width -= 68;
        }
        idx += p.t_axis.size();

        header.setSpan(1, idx, 1, 2);
        header.setItem(1, idx, new_item("Скорость"));

        header.setItem(2, idx + 0, new_item("Значение"));
        header.setColumnWidth(idx + 0, 65);
        width -= 65;
        header.setItem(2, idx + 1, new_item("Единицы"));
        header.setColumnWidth(idx + 1, width);
    }

    header.setRowHeight(0, 25);
    header.setRowHeight(1, 25);
    header.setRowHeight(1, 25);
}

QString ProgramModelHeader::title(program const& p, std::size_t type, std::size_t id) noexcept
{
    switch(type)
    {
    case FcP:
        return QString("ПЧ №%1, Мощность, кВт").arg(id + 1);
    case FcI:
        return QString("ПЧ №%1, Ток, А").arg(id + 1);
    case Spin:
        return QString("Вращение по %1, об/мин").arg(p.s_axis[id]);
    case TargetPos: {
        bool spin = std::find(p.s_axis.begin(), p.s_axis.end(), p.t_axis[id]) != p.s_axis.end();
        return QString("Позиционирование по %1, %2").arg(p.t_axis[id]).arg(spin ? "град" : "мм"); }
    case TargetSpeed: {
        return QString("Скорость позиционирования, %1").arg(id ? "об/мин" : "мм/сек"); }
    }

    return "";
}
