#include "core.h"
#include "alotalot.h"
#include <string.h>
#include <jsonincpp/string_representation.h>
#include <assert.h>

namespace nytl {
    struct LocalVarValue {
        bool is_json = false;
        std::string EL_name;
        const json::JSON* JSON_subval = NULL;
    };

    /* Expression Execution Frame */
    struct EEFrame {
        const json::JSON& expr;
        LocalVarValue& result;
        LocalVarValue temp_ret;
        size_t chain_el = 0;

        EEFrame(const json::JSON &expr, LocalVarValue &result)
            : expr(expr),
              result(result) {
        }

        void descend(const json::JSON& what, const global_elem_set_t& global_elems) {
            if (result.is_json) {
                const json::JSON& P = *result.JSON_subval;
                if (P.isArray() && what.isInteger()) {
                    const std::vector<json::JSON>& arr_p = P.asArray();
                    int64_t ind_w = what.asInteger().get_int();
                    if (!(ind_w > 0 && ind_w < arr_p.size()))
                        THROW("Expression \"array[integer]\" caused out-of-bound situation");
                    result = LocalVarValue{true, "", &arr_p[ind_w]};
                } else if (P.isDictionary() && what.isString()) {
                    const std::map<std::string, json::JSON>& dict_p = P.asDictionary();
                    const std::string& key_w = what.asString();
                    if (dict_p.count(key_w) != 1)
                        THROW("No such key exception (" + key_w + ")");
                    result = LocalVarValue{true, "", &dict_p.at(key_w)};
                } else
                    THROW("Incorrect type of \"json[json]\" expression. Unallowed signature of [] operator");
            } else {
                if (!what.isString())
                    THROW("Expression \"element[X]\" allowed only if X is string (json object)");
                if (!isUname(what.asString()))
                    THROW("Expression \"element[str]\" has incorrect str (" + what.asString() + ")");
                result.EL_name += ("." + what.asString());
                if (global_elems.count(result.EL_name) != 1)
                    THROW("Can't descend. No such element (" + result.EL_name + ")");
            }
        }

        uptr<EEFrame> toMe(bool returned, const global_elem_set_t& global_elems,
                const std::vector<LocalVarValue>& local_vars) {
            if (returned) {
                if (!temp_ret.is_json)
                    THROW("Expression \"X[ element ]\" is not allowed");
                assert(temp_ret.JSON_subval);
                descend(*(temp_ret.JSON_subval), global_elems);
            } else {
                assert(expr.isDictionary());
                const json::JSON& val = expr["V"];
                if (val.isInteger()) {
                    size_t lv_ind = val.asInteger().get_int();
                    assert(lv_ind < local_vars.size());
                    result = local_vars[lv_ind];
                } else if (val.isString()) {
                    std::string cur_el_name_str = expr["V"].asString();
                    if (global_elems.count(cur_el_name_str) != 1)
                        THROW("Bad expression, no such element (" + cur_el_name_str + ")");
                    result = LocalVarValue{false, cur_el_name_str, NULL};
                } else
                    assert(false);
            }
            const std::vector<json::JSON>& chain = expr["C"].asArray();
            while (true) {
                if (chain_el >= chain.size())
                    return NULL;
                const json::JSON& t = chain[chain_el++];
                if (t.isDictionary())
                    return std::make_unique<EEFrame>(t, temp_ret);
                descend(t, global_elems);
            }
        }
    };

    /* No new JSON object will ever be created, I have N root json arguments,
     * all the other json variables are subtrees of them. With one exception: Key iterators for arrays
     * and dictionaries. They are stored in json in rendering stack */
    LocalVarValue rendering_core_execute_expression(const global_elem_set_t& global_elems,
        const std::vector<LocalVarValue>& local_vars, const json::JSON& expr) {

        // todo: check if root element exists (if root value is not local variable, then it is element of package)
        bool returned = false;
        std::vector<uptr<EEFrame>> stack;
        LocalVarValue result;
        stack.push_back(std::make_unique<EEFrame>(expr, result));
        while (!stack.empty()) {
            EEFrame& cur = *stack.back();
            uptr<EEFrame> todo = cur.toMe(returned, global_elems, local_vars);
            returned = !(bool)todo;
            if (todo)
                stack.push_back(mv(todo));
            else
                stack.pop_back();
        }
        return result;
    }

    struct Ditch {
        std::string result;
        size_t cur_line_width = 0;

        /* Fix idea: get rid of newlined_somewhere */
        void append(const std::string& text, size_t wsp_before_newlines, bool& newlined_somewhere) {
            size_t n = result.size();
            size_t m = text.size();
            result.reserve(n + m);
            for (size_t i = 0; i < m; i++) {
                result += text[i];
                if (text[i] == '\n') {
                    result.resize(result.size() + wsp_before_newlines, ' ');
                    cur_line_width = wsp_before_newlines;
                } else {
                    cur_line_width++;
                }
            }
        }
    };

#define RFrame_passed const global_elem_set_t& elem_ns, Ditch& result, const std::function<std::string(std::string)>& escape
    /* Rendering Frame */
    struct RFrame {
        size_t wsp_before_newlines = 0;
        bool newlined_somewhere = false;

        void append(const std::string& text, Ditch& ditch) {
            ditch.append(text, wsp_before_newlines, newlined_somewhere);
        }

        explicit RFrame(size_t multiline_put_start)
            : wsp_before_newlines(multiline_put_start) {
        }

        virtual uptr<RFrame> toMe(bool returned, RFrame_passed) {assert(false);}

        virtual ~RFrame() = default;
    };

    struct RFrame_OverParts : public RFrame{
        std::string name;
        std::vector<LocalVarValue> passed_args;
        /* This parameter incapsulates `cur_line_width` at some point for multiline `put-parts` */
        /* main iterator of this frame. Persistent across control returns */
        size_t part_to_do = 0;

        RFrame_OverParts(const std::string &name, const std::vector<LocalVarValue>& passed_args,
            size_t multiline_put_start)
            : RFrame(multiline_put_start), name(name),
              passed_args(passed_args) {
        }

        uptr<RFrame> toMe(bool returned, RFrame_passed) override;
    };

    struct RFrame_OverJSON: public RFrame {
        const ElementPart::when_for_put_S& part;
        /* During the course of iteration execution, given arg list will expand and shrink back */
        std::vector<LocalVarValue> saved_args_plus_iter;

        RFrame_OverJSON(const ElementPart::when_for_put_S &part, size_t multiline_put_start,
            const std::vector<LocalVarValue> &saved_args)
            : RFrame(multiline_put_start), part(part),
              saved_args_plus_iter(saved_args) {
            if (part.where_key_var > -1)
                this->saved_args_plus_iter.emplace_back();
            if (part.where_value_var > -1)
                this->saved_args_plus_iter.emplace_back();
        }
    };

    struct RFrame_OverArray : public RFrame_OverJSON {
        const std::vector<json::JSON>& arr;
        size_t it = 0;
        /* Crutch. I can't pass simple integer as nytl local variable, I need persistent json wrapper */
        json::JSON additional_json_wrapper;

        RFrame_OverArray(const ElementPart::when_for_put_S& part, size_t multiline_put_start, const std::vector<LocalVarValue> &saved_args,
            const std::vector<json::JSON> &arr): RFrame_OverJSON(part, multiline_put_start, saved_args),
              arr(arr) {
            if (part.where_key_var >= 0)
                additional_json_wrapper = json::JSON(json::Integer(0l));
        }

        uptr<RFrame> toMe(bool returned, const global_elem_set_t &elem_ns, Ditch &result,
            const std::function<std::string(std::string)> &escape) override;
    };

    struct RFrame_OverDictionary: public RFrame_OverJSON {
        const std::map<std::string, json::JSON>& dict;
        std::map<std::string, json::JSON>::const_iterator it;
        /* Crutch */
        json::JSON addition_json_wrapper;

        RFrame_OverDictionary(const ElementPart::when_for_put_S& part, size_t multiline_put_start, const std::vector<LocalVarValue> &saved_args_plus_iter,
            const std::map<std::string, json::JSON> &dict): RFrame_OverJSON(part, multiline_put_start, saved_args_plus_iter),
              dict(dict) {
            it = dict.begin();
            if (part.where_key_var >= 0)
                addition_json_wrapper = json::JSON("");
        }

        uptr<RFrame> toMe(bool returned, const global_elem_set_t &elem_ns, Ditch &result,
            const std::function<std::string(std::string)> &escape) override;
    };

    /* Rendering Frame */
    uptr<RFrame> RFrame_OverParts::toMe(bool returned, const global_elem_set_t &elem_ns, Ditch &result,
        const std::function<std::string(std::string)> &escape) {
        if (!returned)
            if ((elem_ns.count(name) != 1) || (!elem_ns.at(name).is_element))
                THROW("Can't render. No such element (" + name + ")");
        const Element& el = *elem_ns.at(name).when_element;
        if (!returned) {
            /* Continue to do checks */
            /* hidden elements (internal) do not need any check */
            if (!el.is_hidden) {
                size_t n = el.arguments.size();
                ASSERT(n == passed_args.size(), "Argument count mismatch");
                for (size_t i = 0; i < n; i++) {
                    if (el.arguments[i].type == json::true_symbol) {
                        if (!passed_args[i].is_json)
                            THROW("Expected json element argument, got element");
                    } else {
                        // If not json is expected, element must be expected
                        assert(el.arguments[i].isArray());
                        if (passed_args[i].is_json)
                            THROW("Expected element element arguemnt, got json");
                        const std::string& passed_el_as_arg = passed_args[i].EL_name;
                        if ((elem_ns.count(passed_el_as_arg) != 1) || !elem_ns.at(passed_el_as_arg).is_element)
                            THROW("No such element, can't compare signatures of argument value (" + passed_el_as_arg + ")");
                        const Element& arg_element = elem_ns.at(passed_el_as_arg).when_element.operator*();
                        if(el.arguments[i].asArray() != arg_element.arguments)
                            THROW("Signature of argument " + std::to_string(i) + " does not match");
                    }
                }
            }
        }
        if (el.base) {
            assert(!returned);
            assert(passed_args.size() == 1);
            const json::JSON* X = passed_args[0].JSON_subval;
            assert(X);
            if (name == "jsinsert") {
                std::string pure_json = json::generate_str(*X, json::print_pretty);
                rstrip(pure_json);
                append(pure_json, result);
            } else if (name == "jesc") {
                std::string escaped_json = escape(json::generate_str(*X, json::print_pretty));
                rstrip(escaped_json);
                append(escaped_json, result);
            } else if (name == "jesccomp") {
                append(escape(json::generate_str(*X, json::print_compact)), result);
            } else if (name == "str2text") {
                ASSERT(X->isString(), "str2text takes json string");
                append(escape(X->asString()), result);
            } else if (name == "str2code") {
                ASSERT(X->isString(), "str2code takes json string");
                append(X->asString(), result);
            }
            return NULL;
        }
        while (true) {
            if (part_to_do == el.parts.size())
                return NULL;
            const ElementPart& cur_part = el.parts[part_to_do++];
            if (cur_part.type == ElementPart::p_code) {
                const ElementPart::when_code_S& pt = cur_part.when_code;
                append(pt.lines, result);
            } else if (cur_part.type == ElementPart::p_put) {
                const ElementPart::when_put_S& pt = cur_part.when_put;
                LocalVarValue called_element_expv = rendering_core_execute_expression(elem_ns, passed_args, pt.called_element);
                ASSERT(!called_element_expv.is_json, "Can't PUT json variable");
                size_t AN = pt.passed_arguments.size();
                std::vector<LocalVarValue> passed_arguments_expv(AN);
                for (size_t i = 0; i < AN; i++)
                    passed_arguments_expv[i] = rendering_core_execute_expression(elem_ns, passed_args, pt.passed_arguments[i]);
                return std::make_unique<RFrame_OverParts>(called_element_expv.EL_name, passed_arguments_expv,
                                                          result.cur_line_width);
            } else if (cur_part.type == ElementPart::p_for_put) {
                const ElementPart::when_for_put_S& pt = cur_part.when_for_put;
                LocalVarValue iting_over = rendering_core_execute_expression(elem_ns, passed_args, pt.ref_over);
                ASSERT(iting_over.is_json, "Can't iterate over element");
                const json::JSON& container = *iting_over.JSON_subval;
                if (container.isArray()) {
                    return std::make_unique<RFrame_OverArray>(pt, result.cur_line_width, passed_args, container.asArray());
                } else if (container.isDictionary()) {
                    return std::make_unique<RFrame_OverDictionary>(pt, result.cur_line_width, passed_args, container.asDictionary());
                } else
                    THROW("Can't iterate over non-natalistic jsobject");
            } else if (cur_part.type == ElementPart::p_ref_put) {
                const ElementPart::when_ref_put_S& pt = cur_part.when_ref_put;
                std::vector<LocalVarValue> more_variables(passed_args.size() + 1);
                std::copy(passed_args.begin(), passed_args.end(), more_variables.begin());
                more_variables.back() = rendering_core_execute_expression(elem_ns, passed_args, pt.ref_over);
                return std::make_unique<RFrame_OverParts>(pt.internal_element, more_variables, result.cur_line_width);
            }
        }
    }

    uptr<RFrame> RFrame_OverArray::toMe(bool returned, RFrame_passed) {
        if (it >= arr.size())
            return NULL;
        if (returned && part.line_feed)
            append("\n", result);
        if (part.where_key_var > -1) {
            additional_json_wrapper.asInteger() = json::Integer((int64_t)it);
            saved_args_plus_iter[part.where_key_var] = {true, "", &additional_json_wrapper};
        }
        if (part.where_value_var > -1) {
            saved_args_plus_iter[part.where_value_var] = {true, "", &(arr[it])};
        }
        it++;
        return std::make_unique<RFrame_OverParts>(part.internal_element, saved_args_plus_iter, wsp_before_newlines);
    }

    uptr<RFrame> RFrame_OverDictionary::toMe(bool returned, RFrame_passed) {
        if (it == dict.end())
            return NULL;
        if (returned && part.line_feed)
            append("\n", result);
        if (part.where_key_var > -1) {
            addition_json_wrapper.asString() = it->first;
            saved_args_plus_iter[part.where_key_var] = {true, "", &addition_json_wrapper};
        }
        if (part.where_value_var > -1) {
            saved_args_plus_iter[part.where_value_var] = {true, "", &it->second};
        }
        ++it;
        return std::make_unique<RFrame_OverParts>(part.internal_element, saved_args_plus_iter, wsp_before_newlines);
    }

    std::string rendering_core(const std::string& entry_func, const std::vector<const json::JSON*>& entry_arguments,
                               const global_elem_set_t& elem_ns, const std::function<std::string(std::string)>& escape)
    {
        Ditch result;

        std::vector<uptr<RFrame>> stack;
        {
            size_t AN = entry_arguments.size();
            std::vector<LocalVarValue> entry_arguments_conv(AN);
            for (size_t i = 0; i < AN; i++)
                entry_arguments_conv[i] = {true, "", entry_arguments[i]};
            stack.push_back(std::make_unique<RFrame_OverParts>(entry_func, entry_arguments_conv, 0));
        }
        bool returned = false;
        while (!stack.empty()) {
            uptr<RFrame> ret = stack.back()->toMe(returned, elem_ns, result, escape);
            returned = !(bool)ret;
            if (ret)
                stack.push_back(mv(ret));
            else
                stack.pop_back();
        }
        assert(returned);
        return result.result;
    }
}
