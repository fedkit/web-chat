#ifndef IU9_CA_WEB_CHAT_LIB_DEBUG_H
#define IU9_CA_WEB_CHAT_LIB_DEBUG_H

#define debug_print_json(x) printf("%s\n", json::generate_str(x, json::print_pretty).c_str())

#endif
