#include "baza.h"
#include "baza_inter.h"

#include <errno.h>
#include <string.h>

namespace een9 {
    ServerError::ServerError(const std::string &err, const std::string &file, const std::string &func, int line){
        WHAT = "Error occured in function " + func + " (line " + std::to_string(line) + " of " +
           file + ")\nError: " + err;
    }

    const char * ServerError::what() const noexcept {
        return WHAT.c_str();
    }

    std::string prettyprint_errno(const std::string &pref) {
        const char* d = strerrorname_np(errno);
        return pref.empty() ? std::string(d) : std::string(pref) + ": " + d;
    }

    /* This function is VITAL */
    bool strIn(const std::string &str, const char *arr[]) {
        for (const char** elPtr = arr; *elPtr != NULL; elPtr += 1) {
            if (str == (*elPtr))
                return true;
        }
        return false;
    }

    bool endsWith(const std::string &a, const std::string &b) {
        if (b.size() > a.size())
            return false;
        return std::equal(a.end() - (ssize_t)b.size(), a.end(), b.begin());
    }

    bool beginsWith(const std::string &a, const std::string &b) {
        if (b.size() > a.size())
            return false;
        return std::equal(a.begin(), a.begin() + (ssize_t)b.size(), b.begin());
    }

    std::string getSubstring(const std::string &str, size_t A, size_t B) {
        ASSERT(A <= B && B <= str.size(), "Incorrect substring segment");
        return str.substr(A, B - A);
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
}
