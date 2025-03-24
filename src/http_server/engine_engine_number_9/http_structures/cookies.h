#ifndef HTTP_STRUCTURES_COOKIES_H
#define HTTP_STRUCTURES_COOKIES_H

#include <string>
#include <vector>
#include "../baza.h"
#include <map>

namespace een9 {
    bool isCookieName(const std::string& str);

    bool isCookieValue(const std::string& str);

    /* Throws een9::ServerError on failure */
    std::vector<std::pair<std::string, std::string>> parseCookieHeader(const std::string& hv);

    /* Header is header. Throws een9::ServerError on failure. Concatenates output of een9::parseCookieHeader */
    std::vector<std::pair<std::string, std::string>>
    findAllClientCookies(const std::vector<std::pair<std::string, std::string>>& header);

    /* Can throw een9::ServerError (if check for a value failed). res_header_lines is mutated accordingle */
    void set_cookie(const std::vector<std::pair<std::string, std::string>>& new_cookies,
        std::vector<std::pair<std::string, std::string>>& res_header_lines);
}

#endif
