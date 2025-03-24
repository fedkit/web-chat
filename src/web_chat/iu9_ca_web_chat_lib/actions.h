#ifndef IU9_CA_WEB_CHAT_ACTIONS_H
#define IU9_CA_WEB_CHAT_ACTIONS_H

#include <jsonincpp/jsonobj.h>

namespace iu9cawebchat {
    void run_website(const json::JSON& config);
    void initialize_website(const json::JSON& config, const std::string& root_pw);
}

#endif
