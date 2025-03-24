#include "find_db.h"

namespace iu9cawebchat{
    int find_db_sqlite_file_path(const json::JSON& config, std::string& res_path) {
        try {
            const std::string& type = config["database"]["type"].asString();
            if (type != "sqlite3")
                return -1;
            const std::string& path = config["database"]["file"].asString();
            if (path.empty() || path[0] == ':')
                return -1;
            res_path = path;
        } catch (const json::misuse& e) {
            return -1;
        }
        return 0;
    }
}
