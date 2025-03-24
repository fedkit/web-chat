#include "alotalot.h"

#include <algorithm>
#include <errno.h>
#include <string.h>
#include <vector>

namespace nytl {
    FUp::FUp(const std::string &err, const std::string &file, const std::string &func, int line){
        WHAT = "Error occured in function " + func + " (line " + std::to_string(line) + " of " +
           file + ")\nError: " + err;
    }

    const char * FUp::what() const noexcept {
        return WHAT.c_str();
    }

    std::string prettyprint_errno(const std::string &pref) {
        const char* d = strerrorname_np(errno);
        return pref.empty() ? std::string(d) : std::string(pref) + ": " + d;
    }

    bool endsIn(const std::string &a, const std::string &b) {
        if (b.size() > a.size())
            return false;
        return std::equal(a.end() - (ssize_t)b.size(), a.end(), b.begin());
    }

    std::string throwout_postfix(const std::string &a, size_t bsz) {
        return a.substr(0, a.size() >= bsz ? a.size() - bsz : 0);
    }

    bool isALPHA(char ch) {
        return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
    }

    bool isNUM(char ch) {
        return '0' <= ch && ch <= '9';
    }

    bool isUNCHAR(char ch) {
        return isALPHA(ch) || isNUM(ch) || ch == '-' || ch == '_';
    }

    bool isUNCHARnonNUM(char ch) {
        return isALPHA(ch) || ch == '-' || ch == '_';
    }

    bool isSPACE(char ch) {
        return ch == ' ' || ch == '\r' || ch == '\t' || ch == '\n';
    }

    bool isUname(const std::string &str) {
        if (str.empty() || str == "_")
            return false;
        if (isNUM(str[0]))
            return false;
        for (char ch: str)
            if (!isUNCHAR(ch))
                return false;
        return true;
    }

    bool is_uname_dotted_sequence(const std::string& uinp) {
        if (uinp.empty())
            return false;
        std::vector<std::string> r = {""};
        for (char ch: uinp) {
            if (ch == '.') {
                r.emplace_back();
            } else {
                r.back() += ch;
            }
        }
        for (const std::string& c: r)
            if (!isUname(c))
                return false;
        return true;
    }

    std::string make_uppercase(const std::string &source) {
        std::string result(source);
        for (size_t i = 0; i < source.size(); i++) {
            char ch = source[i];
            if ('a' <= ch && ch <= 'z')
                result[i] = (char)(ch - 'a' + 'A');
        }
        return result;
    }

    void rstrip(std::string &str) {
        while (!str.empty() && isSPACE(str.back()))
            str.resize(str.size() - 1);
    }
}
