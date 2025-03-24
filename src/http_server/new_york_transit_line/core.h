#ifndef NEW_YORK_TRANSIT_LINE_CORE_H
#define NEW_YORK_TRANSIT_LINE_CORE_H

#include "templater.h"
#include <functional>

/* Do not export this header */

namespace nytl {
    void debug_print_templater(const Templater& T);

    /* ============== For parsing =============================*/
    void parse_bare_file(const std::string& filename, const std::string& content,
            global_elem_set_t& result);

    void parse_special_file(const std::string& filename, const std::string& content,
        global_elem_set_t& result, TemplaterSettings& syntax);

    /* =================== For rendering ====================*/
    std::string rendering_core(const std::string& entry_func, const std::vector<const json::JSON*>& entry_arguments,
        const global_elem_set_t& elem_ns, const std::function<std::string(std::string)>& escape);
}

#endif
