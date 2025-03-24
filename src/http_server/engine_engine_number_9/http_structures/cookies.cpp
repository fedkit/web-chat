#include "cookies.h"
#include "../baza_inter.h"

#include "grammar.h"

namespace een9 {
    bool isSPACE(char ch) {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }

    bool isALPHANUM(char ch) {
        return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ('0' <= ch && ch <= '9');
    }

    bool isToken(const std::string &str) {
        for (char ch : str) {
            if (!(isALPHANUM(ch)
                || ch == '!' || ch == '#' || ch == '$' || ch == '%' || ch == '&' || ch == '\'' || ch == '*'
                || ch == '+' || ch == '-' || ch == '.' || ch == '^' || ch == '_' || ch == '`' || ch == '|'
                || ch == '~' ))
                return false;
        } // '*+-.^_`|~
        return !str.empty();
    }

    bool isCookieName(const std::string &str) {
        return isToken(str);
    }


    bool isCookieValue(const std::string &str) {
        for (char ch : str) {
            if (ch <= 32 || ch == 0x22 || ch == 0x2C || ch == 0x3B || ch == 0x5C || ch >= 0x7F)
                return false;
        }
        return true;
    }

    std::vector<std::pair<std::string, std::string>> parseCookieHeader(const std::string &hv) {
        std::vector<std::pair<std::string, std::string>> result;
        size_t pos = 0;
        auto skip_ows = [&]() {
            while (hv.size() > pos && isSPACE(hv[pos]))
                pos++;
        };
        auto read_to_space_or_eq = [&]() -> std::string {
            size_t S = pos;
            while (hv.size() > pos && !isSPACE(hv[pos]) && hv[pos] != '=')
                pos++;
            return hv.substr(S, pos - S);
        };
        auto read_to_space_or_semc = [&]() -> std::string {
            size_t S = pos;
            while (hv.size() > pos && !isSPACE(hv[pos]) && hv[pos] != '"' && hv[pos] != ';')
                pos++;
            return hv.substr(S, pos - S);
        };

        auto isThis = [&](char ch) -> bool {
            return pos < hv.size() && hv[pos] == ch;
        };
        skip_ows();
        while (pos < hv.size()) {
            if (!result.empty()) {
                if (!isThis(';'))
                    THROW("Incorrect Cookie header line, missing ;");
                pos++;
                skip_ows();
            }
            std::string name_of_pechenye = read_to_space_or_eq();
            // ASSERT(isCookieName(name_of_pechenye), "Incorrect Cookie name");
            skip_ows();
            if (!isThis('='))
                THROW("Incorrect Cookie header line, missing =");
            pos++;
            skip_ows();
            std::string value_of_pechenye = read_to_space_or_semc();
            // ASSERT(isCookieValue(value_of_pechenye), "Incorrect Cookie value");
            result.emplace_back(name_of_pechenye, value_of_pechenye);
            skip_ows();
        }
        return result;
    }

    std::vector<std::pair<std::string, std::string>>
    findAllClientCookies(const std::vector<std::pair<std::string, std::string>>& header) {
        std::vector<std::pair<std::string, std::string>> result;
        for (const std::pair<std::string, std::string>& line: header) {
            try {
                if (line.first == "Cookie") {
                    std::vector<std::pair<std::string, std::string>> new_cookies = parseCookieHeader(line.second);
                    result.reserve(result.size() + new_cookies.size());
                    result.insert(result.end(), new_cookies.begin(), new_cookies.end());
                }
            } catch (const std::exception& e) {}
        }
        return result;
    }

    void set_cookie(const std::vector<std::pair<std::string, std::string>>& new_cookies,
                    std::vector<std::pair<std::string, std::string>>& res_header_lines) {
        for (const std::pair<std::string, std::string>& cookie : new_cookies) {
            ASSERT_pl(isCookieName(cookie.first) && isCookieValue(cookie.second));
            res_header_lines.emplace_back("Set-Cookie", cookie.first + "=" + cookie.second + ";SameSite=Strict;Path=/");
        }
    }
}
