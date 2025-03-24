#include "response_gen.h"
#include "../baza_inter.h"
#include <assert.h>
#include <string.h>


namespace een9 {
    std::string form_http_server_response_header(const char* code,
        const std::vector<std::pair<std::string, std::string>>& headers) {
        assert(strlen(code) == 3);
        std::string result = std::string("HTTP/1.0 ") + code + " " + (code[0] < '4' ? "OK" : "ERROR") + "\r\n";
        for (auto& p: headers)
            result += (p.first + ": " + p.second + "\r\n");
        return result;
    }

    std::string form_http_server_response_header_only(const char* code,
        const std::vector<std::pair<std::string, std::string>>& headers) {
        return form_http_server_response_header(code, headers) + "\r\n";
    }

    std::string form_http_server_response_with_body(const char* code,
        const std::vector<std::pair<std::string, std::string>>& headers,
        const std::string& body)
    {
        std::string result = form_http_server_response_header(code, headers)
        + "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        return result;
    }

    /* Message from server to client */
    std::string form_http_server_response_200(const std::string& Content_Type, const std::string& body) {
        return form_http_server_response_with_body("200", {
            {"Content-Type", Content_Type}
        }, body);
    }

    std::string form_http_server_response_404(const std::string& Content_Type, const std::string& body) {
        return form_http_server_response_with_body("404", {
            {"Content-Type", Content_Type}
        }, body);
    }

    std::string form_http_server_response_303(const std::string& Location) {
        return form_http_server_response_header_only("303", {{"Location", Location}});
    }

    std::string form_http_server_response_303_spec_head(const std::string &Location,
        const std::vector<std::pair<std::string, std::string>>& headers) {
        std::vector<std::pair<std::string, std::string>> cp = headers;
        cp.emplace_back("Location", Location);
        return form_http_server_response_header_only("303", cp);
    }
}
