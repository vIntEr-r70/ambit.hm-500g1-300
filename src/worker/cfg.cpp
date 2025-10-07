#include "cfg.h"

aem::time_span cfg::uport_response_timeout = aem::time_span::ms(20);

aem::time_span cfg::trm_polling_frequency = aem::time_span::ms(100);
aem::time_span cfg::plc_polling_frequency = aem::time_span::ms(100);
aem::time_span cfg::mk_polling_frequency = aem::time_span::ms(100);

aem::time_span cfg::cnc_polling_frequency = aem::time_span::ms(100);

float cfg::cnc_position_accuracy = 0.00001f;
float cfg::cnc_position_compare = 0.1f;

aem::uint16 cfg::rpc_server_port = 4000;
aem::uint16 cfg::nf_server_port = 4001;

aem::time_span cfg::hw_move_ctrl_timeout = aem::time_span::s(2);
