#include "login_cookie.h"
#include <jsonincpp/string_representation.h>
#include "str_fields.h"
#include <engine_engine_number_9/baza_throw.h>
#include <engine_engine_number_9/http_structures/cookies.h>
#include <string.h>

namespace iu9cawebchat {
    bool is_login_cookie(const std::pair<std::string, std::string>& any_cookie_encoded) {
        return een9::beginsWith(any_cookie_encoded.first, "login_") && !any_cookie_encoded.second.empty();
    }

    /* Should be verified with iu9cawebchat::is_login_cookie. Can throw std::exception anyway */
    LoginCookie decode_login_cookie(const std::pair<std::string, std::string>& login_cookie_encoded) {
        const std::string& ft = login_cookie_encoded.first;
        size_t bg = strlen("login_");
        een9_ASSERT_pl(ft.size() >= bg);
        size_t s_ = bg;
        while (s_ < ft.size() && ft[s_] != '_')
            s_++;
        een9_ASSERT_pl(s_ + 1 < ft.size())
        uint64_t sec = std::stoull(ft.substr(bg, s_ - bg));
        uint64_t nsec = std::stoull(ft.substr(s_ + 1, ft.size() - s_ - 1));
        een9_ASSERT_pl(nsec < 1000000000);
        const json::JSON cnt = json::parse_str_flawless(base64_decode(login_cookie_encoded.second));
        std::string nickname = cnt[0].asString();
        std::string password = cnt[1].asString();
        return LoginCookie{{(time_t)sec, (time_t)nsec}, nickname, password};
    }

    LoginCookie create_login_cookie(const std::string& nickname, const std::string& password) {
        ns_time moment;
        int ret = clock_gettime(CLOCK_REALTIME, &moment);
        een9_ASSERT_on_iret(ret, "Can't get time");
        return {moment, nickname, password};
    }

    std::pair<std::string, std::string> encode_login_cookie(const LoginCookie& cookie) {
            json::JSON cnt;
            cnt[1].asString() = cookie.password;
            cnt[0].asString() = cookie.nickname;
        return {"login_" + std::to_string(cookie.login_time.tv_sec) + "_" + std::to_string(cookie.login_time.tv_nsec),
            base64_encode(json::generate_str(cnt, json::print_compact))};
    }

    bool login_cookie_age_less(const LoginCookie& A, const LoginCookie& B) {
        if (A.login_time.tv_sec < B.login_time.tv_sec)
            return true;
        if (A.login_time.tv_sec > B.login_time.tv_sec)
            return false;
        return A.login_time.tv_nsec < B.login_time.tv_nsec;
    }

    std::vector<LoginCookie> select_login_cookies(const std::vector<std::pair<std::string, std::string>> &cookie_list) {
        std::vector<LoginCookie> needed;
        try {
            for (const std::pair<std::string, std::string>& C: cookie_list)
                if (is_login_cookie(C))
                    needed.push_back(decode_login_cookie(C));
        } catch (const std::exception& e) {
            return {};
        }
        return needed;
    }

    size_t select_oldest_login_cookie(const std::vector<LoginCookie> &login_cokie_list) {
        size_t oldest = 0;
        for (size_t i = 1; i < login_cokie_list.size(); i++)
            if (login_cookie_age_less(login_cokie_list[i], login_cokie_list[oldest]))
                oldest = i;
        return login_cokie_list.empty() ? 0 : oldest;
    }

    void add_set_cookie_headers_to_login(const std::vector<LoginCookie> &old_login_cookies,
        std::vector<std::pair<std::string, std::string>> &response_headers, const LoginCookie& new_login_cookie) {
        add_set_cookie_headers_to_logout(old_login_cookies, response_headers);
        een9::set_cookie({encode_login_cookie(new_login_cookie)}, response_headers);
    }

    void add_set_cookie_headers_to_logout(const std::vector<LoginCookie> &old_login_cookies,
        std::vector<std::pair<std::string, std::string>> &response_headers) {
        std::vector<std::pair<std::string, std::string>> anti_cookies(old_login_cookies.size());
        for (size_t i = 0; i < old_login_cookies.size(); i++) {
            anti_cookies[i] = {"login_" + std::to_string(old_login_cookies[i].login_time.tv_sec) +
                "_" + std::to_string(old_login_cookies[i].login_time.tv_nsec), ""};
        }
        een9::set_cookie(anti_cookies, response_headers);
    }
}
