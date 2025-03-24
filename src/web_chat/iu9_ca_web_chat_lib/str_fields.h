#ifndef IU9_CA_WEB_CHAT_SRC_WEB_CHAT_STR_FIELDS_CHECK_H
#define IU9_CA_WEB_CHAT_SRC_WEB_CHAT_STR_FIELDS_CHECK_H

#include <string>
#include <stdint.h>

namespace iu9cawebchat {
    bool isALPHA(char ch);
    bool isNUM(char ch);
    bool isUNCHAR(char ch);
    bool isSPACE(char ch);

    bool is_orthodox_string(const std::string& str);

    bool check_password(const std::string& pwd);
    bool check_strong_password(const std::string& pwd);
    bool check_name(const std::string& name);
    bool check_nickname(const std::string& nickname);

    std::string base64_encode(const std::string& source);

    /* Кусаеца */
    std::string base64_decode(const std::string& source);
}

#endif
