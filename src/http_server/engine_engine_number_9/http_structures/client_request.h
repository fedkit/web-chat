#ifndef ENGINE_ENGINE_NUMBER_9_HTTP_STRUCTURES_CLIENT_REQUEST_H
#define ENGINE_ENGINE_NUMBER_9_HTTP_STRUCTURES_CLIENT_REQUEST_H

#include <vector>
#include <string>
#include <utility>

namespace een9 {
    /* host:port scheme:authority and asterisk types of URI in http request are not supported by een9 */
    struct ClientRequest {
        std::string method;
        std::string uri_path;
        bool has_query = false;
        std::string uri_query;
        std::string http_version;
        std::vector<std::pair<std::string, std::string>> headers;
        bool has_body = false;
        std::string body;
    };
}

#endif
