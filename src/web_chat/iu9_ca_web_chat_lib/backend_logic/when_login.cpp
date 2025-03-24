#include "client_server_interact.h"
#include <engine_engine_number_9/form_data_structure/urlencoded_query.h>
#include <engine_engine_number_9/baza_throw.h>
#include "../str_fields.h"

namespace iu9cawebchat {
    std::string when_page_login(WorkerGuestData& wgd, const json::JSON& config_presentation,
        const een9::ClientRequest& req, const std::vector<LoginCookie>& login_cookies, const json::JSON& userinfo) {
        if (req.method == "POST") {
            std::vector<std::pair<std::string, std::string>> query = een9::split_html_query(req.body);
            int64_t uid = -1;
            std::string nickname;
            std::string password;
            try {
                for (const std::pair<std::string, std::string>& cmp: query) {
                    if (cmp.first == "nickname")
                        nickname = cmp.second;
                    if (cmp.first == "password")
                        password = cmp.second;
                }
                if (!check_nickname(nickname))
                    een9_THROW("/login/accpet-data rejected impossible nickname");
                if (!check_password(password))
                    een9_THROW("/login/accpet-data rejected impossible password");
                uid = find_user_by_credentials(*wgd.db, nickname, password);

            } catch(const std::exception& e){}
            if (uid < 0) {
                printf("Redirecting back to /login because of incorrect credentials\n");
                json::JSON msg_list = jsonify_html_message_list({{"",
                    config_presentation["login"]["incorrect-nickname-or-password"].asString()}});
                return http_R200("login", wgd, {&config_presentation, &userinfo, &msg_list});
            }
            std::vector<std::pair<std::string, std::string>> response_hlines;
            LoginCookie new_login_cookie = create_login_cookie(nickname, password);
            add_set_cookie_headers_to_login(login_cookies, response_hlines, new_login_cookie);
            return een9::form_http_server_response_303_spec_head("/", response_hlines);
        }
        if (req.method != "GET")
            een9_THROW("Bad method");
        json::JSON empty_msg_list = json::JSON(json::array);
        return http_R200("login", wgd, {&config_presentation, &userinfo, &empty_msg_list});
    }
}
