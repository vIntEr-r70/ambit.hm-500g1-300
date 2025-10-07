#pragma once

#include <QWidget>

class ValueSetBool;

namespace aem::net {
    class rpc_tcp_client;
}

class BkiCfgWidget
    : public QWidget
{
public:

    BkiCfgWidget(QWidget*);

private:

    void on_bki_status();

private:

    aem::net::rpc_tcp_client &rpc_;

    ValueSetBool *vsb_;
};
