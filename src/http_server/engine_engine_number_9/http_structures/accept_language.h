#ifndef ENGINE_ENGINE_NUMBER_9_HTTP_STRUCTURES_ACCEPT_LANGUAGE_H
#define ENGINE_ENGINE_NUMBER_9_HTTP_STRUCTURES_ACCEPT_LANGUAGE_H

#include <vector>
#include <string>

namespace een9 {
    /* Returns language ranges, sorted by priority (reverse)
     * throws std::exception if header is incorrect! But it is not guaranteed. Maybe it won't */
    std::vector<std::string> parse_header_Accept_Language(const std::string& AcceptLanguage);
}

#endif
