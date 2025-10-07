#pragma once

#include <aem/time_span.h>
#include <aem/types.h>

struct cfg
{
    static aem::time_span uport_response_timeout;

    static aem::time_span trm_polling_frequency;
    static aem::time_span plc_polling_frequency;
    static aem::time_span mk_polling_frequency;

    static aem::time_span hw_move_ctrl_timeout;

    static aem::time_span cnc_polling_frequency;

    static float cnc_position_accuracy;
    static float cnc_position_compare;

    static aem::uint16 rpc_server_port;
    static aem::uint16 nf_server_port;
};
