#ifndef NEW_YORK_TRANSIT_LINE_TEMPLATER_H
#define NEW_YORK_TRANSIT_LINE_TEMPLATER_H

#include <vector>
#include <string>
#include <jsonincpp/jsonobj.h>
#include <functional>
#include "html_case.h"
#include <memory>
#include <forward_list>

namespace nytl {
    typedef json::JSON expression_t;

    struct ElementPart {
        /* Used with all types */
        enum element_part_type_E {
            p_code,
            /* write statements really mean PUT str2text X */
            p_put,
            p_for_put,
            p_ref_put
        } type = p_code;
        struct when_code_S {
            std::string lines;
        } when_code;
        struct when_put_S {
            expression_t called_element;
            std::vector<expression_t> passed_arguments;
        } when_put;
        struct when_for_put_S {
            expression_t ref_over;
            ssize_t where_key_var = -1;
            ssize_t where_value_var = -1;
            std::string internal_element;
            bool line_feed = true;
        } when_for_put;
        struct when_ref_put_S {
            expression_t ref_over;
            std::string internal_element;
        } when_ref_put;
    };

    struct Element {
        /* Stores signature of element */
        std::vector<json::JSON> arguments;
        /* `base` is true for builtin elements (jesc str2code str2text). Parts for such ' are empty */
        bool base = false;
        bool is_hidden = false;
        std::vector<ElementPart> parts;
    };

    struct TemplaterDetourRules {
        std::string root_dir_path;
        std::string postfix_rule_for_element_cont = ".nytl.html";
        std::string postfix_rule_for_static_files = ".html";
    };

    struct TemplaterSettings {
        TemplaterDetourRules det;
        std::string magic_block_start = "{%";
        std::string magic_block_end = "%}";
        std::function<std::string(std::string)> escape = html_case_espace_string;
    };

    struct TemplaterRegPref {
        int is_element = 0;
        std::unique_ptr<Element> when_element = NULL;
    };

    typedef std::map<std::string, TemplaterRegPref> global_elem_set_t;

    struct Templater {
        TemplaterSettings settings;

        global_elem_set_t elements;

        explicit Templater(TemplaterSettings settings);

        /* Throws exception, derived from std::exception */
        void update();

        /* Throws exception, derived from std::exception */
        std::string render(const std::string& element, const std::vector<const json::JSON*>& arguments) const;
    };
}

#endif
