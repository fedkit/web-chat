#ifndef ENGINE_ENGINE_NUMBER_9_HTTP_STRUCTURES_RESPONSE_GEN_H
#define ENGINE_ENGINE_NUMBER_9_HTTP_STRUCTURES_RESPONSE_GEN_H

#include <vector>
#include <string>


namespace een9 {
    std::string form_http_server_response_header(const char* code,
        const std::vector<std::pair<std::string, std::string>>& headers);

    std::string form_http_server_reponse_header_only(const char* code,
        const std::vector<std::pair<std::string, std::string>>& headers);

    std::string form_http_server_response_with_body(const char* code,
        const std::vector<std::pair<std::string, std::string>>& headers,
        const std::string& body);

    std::string form_http_server_response_200(const std::string& Content_Type, const std::string& body);

    std::string form_http_server_response_404(const std::string& Content_Type, const std::string& body);

    std::string form_http_server_response_303(const std::string& Location);

    std::string form_http_server_response_303_spec_head(const std::string &Location,
        const std::vector<std::pair<std::string, std::string>>& headers);
}

#endif
