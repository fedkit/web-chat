#ifndef IU9_CA_WEB_CHAT_LIB_LOGIN_COOKIE_H
#define IU9_CA_WEB_CHAT_LIB_LOGIN_COOKIE_H

#include <time.h>
#include <string>
#include <vector>

namespace iu9cawebchat {
    typedef struct timespec ns_time;

    struct LoginCookie {
        ns_time login_time;
        std::string nickname;
        std::string password;
    };

    bool is_login_cookie(const std::pair<std::string, std::string>& any_cookie_encoded);
    LoginCookie decode_login_cookie(const std::pair<std::string, std::string>& login_cookie_encoded);
    LoginCookie create_login_cookie(const std::string& nickname, const std::string& password);
    std::pair<std::string, std::string> encode_login_cookie(const LoginCookie& cookie);

    /* (incorrect cookie set is treated as unloginned user ) */
    std::vector<LoginCookie> select_login_cookies(const std::vector<std::pair<std::string, std::string>> &cookie_list);
    size_t select_oldest_login_cookie(const std::vector<LoginCookie>& login_cokie_list);

    /* Populates response headers */
    void add_set_cookie_headers_to_login(const std::vector<LoginCookie>& old_login_cookies,
        std::vector<std::pair<std::string, std::string>>& response_headers, const LoginCookie& new_login_cookie);
    void add_set_cookie_headers_to_logout(const std::vector<LoginCookie>& old_login_cookies,
        std::vector<std::pair<std::string, std::string>>& response_headers);

}

#endif
