#include "client_server_interact.h"

namespace iu9cawebchat {
    std::string when_page_list_rooms(WorkerGuestData& wgd, const json::JSON& config_presentation,
            const een9::ClientRequest& req, const json::JSON& userinfo) {
        if (userinfo.isNull()) {
            return een9::form_http_server_response_303("/login");
        }
        json::JSON initial_chatListUpdResp = poll_update_chat_list_resp(*wgd.db,
            userinfo["uid"].asInteger().get_int(), 0);
        printf("%s\n", json::generate_str(initial_chatListUpdResp, json::print_pretty).c_str());
        return http_R200("list-rooms", wgd, {&config_presentation, &userinfo, &initial_chatListUpdResp});
    }
}
