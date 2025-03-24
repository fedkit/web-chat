#include "templater.h"
#include "alotalot.h"
#include "jsonincpp/string_representation.h"
#include <assert.h>

namespace nytl {
    void debug_print_templater(const Templater& T) {
        printf("===== TEMPLATER INTERNAL RESOURCES =====\n");
        for (auto& p: T.elements) {
            if (!p.second.is_element) {
                printf("=== %s is empty =====\n", p.first.c_str());
                continue;
            }
            printf("=== %s element =====\n", p.first.c_str());
            assert(p.second.when_element);
            const Element& el = *p.second.when_element;
            printf("%s, %s\n", el.base ? "BASE" : "NOT BASE", el.is_hidden ? "HIDDEN" : "NOT HIDDEN");
            if (!el.is_hidden) {
                std::string signature;
                for (const json::JSON& arg_type: el.arguments) {
                    if (!signature.empty())
                        signature += "  ";
                    signature += json::generate_str(arg_type, json::print_compact);
                }
                printf("Signature: %s\n", signature.c_str());
            }
            for (const ElementPart& part: el.parts) {
                if (part.type == ElementPart::p_code) {
                    printf("code:   <b><e><f><o><r><e><><l><f>\n%s\n<a><f><t><e><r><><l><f>\n", part.when_code.lines.c_str());
                } else if (part.type == ElementPart::p_for_put) {
                    const ElementPart::when_for_put_S& P = part.when_for_put;
                    printf("dor cycle call:\ninternal_element: %s,\nref_over:%s,\nwhere_key_var: %ld, where_value_var: %ld, %s\n",
                        P.internal_element.c_str(), json::generate_str(P.ref_over, json::print_pretty).c_str(),
                        P.where_key_var, P.where_value_var, P.line_feed ? "LF" : "NOLF");
                } else if (part.type == ElementPart::p_ref_put) {
                    const ElementPart::when_ref_put_S& P = part.when_ref_put;
                    printf("ref block call:\ninternal_element: %s\nref_over:%s\n",
                        P.internal_element.c_str(), json::generate_str(P.ref_over, json::print_pretty).c_str());
                } else {
                   assert(part.type == ElementPart::p_put);
                    const ElementPart::when_put_S& P = part.when_put;
                    printf("PUT:\ncalled_element: %s\n",
                        json::generate_str(P.called_element, json::print_pretty).c_str());
                    for (size_t i = 0; i < P.passed_arguments.size(); i++) {
                        printf("passed_arguments[%lu] = %s\n", i,
                            json::generate_str(P.passed_arguments[i], json::print_pretty).c_str());
                    }
                }
            }
            printf("=== That was element %s ====\n", p.first.c_str());
        }
        printf("===== DEBUG IS OVER =====\n");
    }
}