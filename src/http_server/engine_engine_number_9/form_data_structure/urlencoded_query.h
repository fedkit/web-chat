#ifndef ENGINE_ENGINE_NUMBER_9_FORM_DATA_STRUCTURE_URLENCODED_QUERY_H
#define ENGINE_ENGINE_NUMBER_9_FORM_DATA_STRUCTURE_URLENCODED_QUERY_H

#include <string>
#include <vector>

namespace een9{
    /* application/x-www-form-urlencoded */
    std::vector<std::pair<std::string, std::string>> split_html_query(const std::string& query);
}

#endif
