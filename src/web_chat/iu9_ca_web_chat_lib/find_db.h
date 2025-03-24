#ifndef IU9_CA_WEB_CHAT_SRC_WEB_CHAT_FIND_DB_H
#define IU9_CA_WEB_CHAT_SRC_WEB_CHAT_FIND_DB_H

#include <jsonincpp/jsonobj.h>

namespace iu9cawebchat {
    int find_db_sqlite_file_path(const json::JSON& config, std::string& res_path);
}

#endif
