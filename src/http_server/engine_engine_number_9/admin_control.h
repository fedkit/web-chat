#ifndef ENGINE_ENGINE_NUMBER_9_ADMIN_CONTROL_H
#define ENGINE_ENGINE_NUMBER_9_ADMIN_CONTROL_H

#include <string>
#include <stdint.h>

namespace een9 {
    struct AdminControlProtMsgRecvCtx {
        const char* magic_string;
        size_t ms_size;
        size_t magic_string_progress = 0;
        size_t body_size_progress = 0;
        uint64_t b_sz = 0;
        std::string body;
        int status = 0;

        explicit AdminControlProtMsgRecvCtx(const char* ms);

        int feedCharacter(char ch);
    };

    struct AdminControlRequestRCtx: public AdminControlProtMsgRecvCtx {
        AdminControlRequestRCtx();
    };

    struct AdminControlResponseRCtx: public AdminControlProtMsgRecvCtx {
        AdminControlResponseRCtx();
    };

    /* From ADMIN to server (will begin with admin_to_server_ms)*/
    std::string generate_admin_control_request(const std::string& content);

    /* From SERVER to admin (will begin with server_to_admin_ms).*/
    std::string generate_admin_control_response(const std::string& content);
}

#endif
