#include "html_case.h"

namespace nytl {
    std::string html_case_espace_string(const std::string &inp) {
        std::string res;
        res.reserve(inp.size());
        for (char ch: inp) {
            switch (ch) {
                case '&':
                    res += "&amp";
                break;
                case '<':
                    res += "&lt";
                break;
                case '>':
                    res += "&gt";
                break;
                case '"':
                    res += "&quot";
                break;
                case '\'':
                    res += "&#39";
                break;
                default:
                    res += ch;
            }
        }
        return res;
    }
}