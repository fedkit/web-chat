#include "urlencoded_query.h"
#include <stdint.h>

namespace een9 {
    std::vector<std::pair<std::string, std::string>> split_html_query(const std::string &query) {
        std::vector<std::pair<std::string, std::string>> result;
        if (query.empty())
            return result;
        result = {{"", ""}};
        bool filling_second = false;
        auto fref = [&]() -> std::string& { return filling_second ? result.back().second : result.back().first; };
        for (size_t i = 0; i < query.size();) {
            if (query[i] == '&') {
                result.emplace_back("", "");
                filling_second = false;
            } else if (query[i] == '=') {
                filling_second = true;
            } else if (query[i] == '+') {
                fref() += ' ';
            } else if (query[i] == '%') {
                if (i + 3 > query.size())
                    return {};
                auto readhex = [&](char ch) -> uint8_t {
                    if ('0' <= ch && ch <= '9')
                        return ch - '0';
                    if ('a' <= ch && ch <= 'h')
                        return ch - 'a' + 10;
                    if ('A' <= ch && ch <= 'H')
                        return ch - 'A' + 10;
                    return 10;
                };
                fref() +=  (char)((readhex(query[i + 1]) << 4) | readhex(query[i + 2]));
                i += 2;
            } else {
                fref() += query[i];
            }
            i++;
        }
        return result;
    }
}
