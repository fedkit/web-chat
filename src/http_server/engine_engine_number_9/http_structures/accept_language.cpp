#include "accept_language.h"
#include <algorithm>
#include "grammar.h"
#include "../baza_inter.h"

namespace een9 {
    bool AcceptLanguageSpec(char ch) {
        return ch == ',' || ch == ';' || ch == '=';
    }

    /* todo: This is one of many places in een9, where bad alloc does not interrupt request,
     * todo:  completely changing response instead. (see cookies and login cookies lol)
     * todo: I have to do something about it. Maybe add more exception types */
    std::vector<std::string> parse_header_Accept_Language(const std::string &AcceptLanguage) {
        size_t n = AcceptLanguage.size();
        struct LR {
            std::string lr;
            float q = 1;
        };
        size_t i = 0;
        auto skipOWS = [&]() {
            while (i < n && isSPACE(AcceptLanguage[i]))
                i++;
        };
        auto isThis = [&](char ch) {
            skipOWS();
            return i >= n ? false : AcceptLanguage[i] == ch;
        };
        auto readTkn = [&]() -> std::string {
            skipOWS();
            if (i >= n)
                return "";
            size_t bg = i;
            while (i < n && !AcceptLanguageSpec(AcceptLanguage[i]) && !isSPACE(AcceptLanguage[i]))
                i++;
            return AcceptLanguage.substr(bg, i - bg);
        };
        std::vector<LR> lrs;
#define myMsg "Bad Accept-Language"
        while (i < n) {
            skipOWS();
            if (i >= n)
                break;
            if (!lrs.empty()) {
                if (isThis(','))
                    i++;
                else
                    break;
            }
            lrs.emplace_back();
            lrs.back().lr = readTkn();
            LR lr{readTkn(), 0};
            if (isThis(';')) {
                i++;
                if (readTkn() != "q")
                    THROW(myMsg);
                if (!isThis('='))
                    THROW(myMsg);
                i++;
                lrs.back().q = std::stof(readTkn());
            }
        }
        std::sort(lrs.begin(), lrs.end(), [](const LR& A, const LR& B) {
            return A.q > B.q;
        });
        std::vector<std::string> result;
        result.reserve(lrs.size());
        for (const LR& lr: lrs)
            result.push_back(lr.lr == "*" ? "" : lr.lr);
        return result;
    }
}
