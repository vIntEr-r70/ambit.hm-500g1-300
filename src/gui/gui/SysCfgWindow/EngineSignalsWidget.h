#pragma once

#include <aem/net/rpc_tcp_client.h>

#include <QWidget>

class QTableView;
class SignalsModel;

class EngineSignalsWidget final
    : public QWidget
{
public:

    EngineSignalsWidget(QWidget *parent) noexcept;

private:

    aem::net::rpc_tcp_client &rpc_;

    QTableView *table_;
    SignalsModel *model_;
};

