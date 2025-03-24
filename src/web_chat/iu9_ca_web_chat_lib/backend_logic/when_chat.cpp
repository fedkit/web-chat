#include "client_server_interact.h"

#include <engine_engine_number_9/baza_throw.h>
#include "../str_fields.h"

namespace iu9cawebchat {
    std::string when_page_chat(WorkerGuestData& wgd, const json::JSON& config_presentation,
        const een9::ClientRequest& req, const json::JSON& userinfo) {

        std::vector<std::string> path_segs = {};
        path_segs.reserve(4);
        if (req.uri_path.empty() || req.uri_path[0] != '/')
            return page_E404(wgd);
        for (char ch: req.uri_path) {
            if (ch == '/')
                path_segs.emplace_back();
            else
                path_segs.back() += ch;
        }
        // Parameter, passed from server to javascript
        std::string chat_nickname;
        int64_t selected_message_id = -1;
        if (path_segs.size() >= 2) {
            chat_nickname = std::move(path_segs[1]);
        }
        if (!check_nickname(chat_nickname))
            return page_E404(wgd);
        bool show_chat_members = (path_segs[0] == "chat-members");
        if (path_segs.size() == 4 && !show_chat_members) {
            if (path_segs[2] != "m")
                return page_E404(wgd);
            selected_message_id = std::stoll(path_segs[3]);
        } else if (path_segs.size() != 2) {
            return page_E404(wgd);
        }

        if (userinfo.isNull())
            return een9::form_http_server_response_303("/");

        RowChat_Content chatInfo;
        try {
            chatInfo = lookup_chat_content_by_nickname(*wgd.db, chat_nickname);
        } catch (const std::exception& e) {
            return page_E404(wgd);
        }
        if (get_role_of_user_in_chat(*wgd.db, userinfo["uid"].asInteger().get_int(), chatInfo.id) == user_chat_role_deleted) {
            return page_E404(wgd);
        }

        json::JSON openedchat;
        openedchat["name"].asString() = chatInfo.name;
        openedchat["nickname"].asString() = chatInfo.nickname;
        openedchat["id"].asInteger() = json::Integer(chatInfo.id);
        // -1 means that nothing was selected
        openedchat["selectedMessageId"].asInteger() = json::Integer(selected_message_id);
        json::JSON initial_chatUpdResp = poll_update_chat_ONE_MSG_resp(*wgd.db, chatInfo.id, selected_message_id);
        if (show_chat_members)
            return http_R200("chat-members", wgd, {&config_presentation, &userinfo, &openedchat, &initial_chatUpdResp});
        return http_R200("chat", wgd, {&config_presentation, &userinfo, &openedchat, &initial_chatUpdResp});
    }
}
