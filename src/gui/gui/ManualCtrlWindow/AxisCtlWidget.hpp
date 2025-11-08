#pragma once

#include <QWidget>

#include <eng/sibus/node.hpp>

class AxisCtlItemWidget;
class QVBoxLayout;
class QHBoxLayout;

class AxisCtlWidget
    : public QWidget 
    , public eng::sibus::node
{
    QVBoxLayout* vL_;
    std::vector<QHBoxLayout*> hL_;

    std::map<char, AxisCtlItemWidget*> axis_;

public:

    AxisCtlWidget(QWidget*) noexcept;

private:

    void add_axis(char, std::string_view);

    void add_axis(char, bool) noexcept;
};
