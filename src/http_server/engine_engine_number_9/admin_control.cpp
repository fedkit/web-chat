#include "admin_control.h"
#include <string.h>
#include <assert.h>

namespace een9 {
    static const char* admin_to_server_ms = "a6m1n 2 server request ~~~";
    static const char* server_to_admin_ms = "server to 4dm1n r3sponse ~~~";

    AdminControlProtMsgRecvCtx::AdminControlProtMsgRecvCtx(const char *ms): magic_string(ms) {
        ms_size = strlen(ms);
    }

    int AdminControlProtMsgRecvCtx::feedCharacter(char ch) {
        assert(status == 0);
        if (magic_string_progress < ms_size) {
            if (magic_string[magic_string_progress] != ch) {
                status = -1;
                return status;
            }
            magic_string_progress++;
        } else if (body_size_progress < 8) {
            uint64_t bt = (uint64_t)(uint8_t)ch;
            b_sz = ((b_sz << 8) | bt);
            body_size_progress++;
            if (body_size_progress == 8 && b_sz > 100000000) {
                status = -1;
                return status;
            }
            body.reserve(b_sz);
        } else {
            body += ch;
            if (body.size() >= b_sz)
                status = 1;
        }
        return status;
    }

    AdminControlRequestRCtx::AdminControlRequestRCtx(): AdminControlProtMsgRecvCtx(admin_to_server_ms) {
    }

    AdminControlResponseRCtx::AdminControlResponseRCtx(): AdminControlProtMsgRecvCtx(server_to_admin_ms) {
    }

    std::string generate_ac_msg_gen_case(const std::string& content, const char* ms) {
        std::string result = ms;
        uint64_t N = content.size();
        for (int i = 0; i < 8; i++) {
            result += (char)(uint8_t)((N & 0xff00000000000000) >> 56);
            N <<= 8;
        }
        result += content;
        return result;
    }

    std::string generate_admin_control_request(const std::string &content) {
        return generate_ac_msg_gen_case(content, admin_to_server_ms);
    }

    std::string generate_admin_control_response(const std::string &content) {
        return generate_ac_msg_gen_case(content, server_to_admin_ms);
    }
}
