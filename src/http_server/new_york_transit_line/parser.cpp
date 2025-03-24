#include "core.h"
#include "alotalot.h"
#include <vector>
#include <assert.h>

namespace nytl {
    size_t first_nw_char(const std::string& str) {
        size_t i = 0;
        for (; i < str.size(); i++)
            if (!isSPACE(str[i]))
                break;
        return i;
    }

    bool is_space_only(const std::string& str) {
        return first_nw_char(str) == str.size();
    }

    std::string clement_lstrip(const std::string& str) {
        size_t gone = 0;
        size_t n = str.size();
        for (size_t i = 0; i < n; i++) {
            if (str[i] == '\n') {
                gone = i + 1;
            } else if (!isSPACE(str[i])) {
                break;
            }
        }
        return str.substr(gone);
    }

    struct ParsingContext {
        std::string text;
        size_t pos = 0;
        size_t column = 0;
        size_t line = 0;
    };

    constexpr int EOFVAL = -999;

    int peep(ParsingContext &ctx) {
        if (ctx.text.size() <= ctx.pos)
            return EOFVAL;
        return ctx.text[ctx.pos];
    }

    char advance(ParsingContext& ctx) {
        if (ctx.text[ctx.pos] == '\n') {
            ctx.line++;
            ctx.column = 0;
        } else {
            ctx.column++;
        }
        return ctx.text[ctx.pos++];
    }

    char skip(ParsingContext& ctx) {
        if (ctx.pos >= ctx.text.size())
            THROW("Unexpected EOF");
        return advance(ctx);
    }

    void skip(ParsingContext& ctx, char ch) {
        if (ctx.pos >= ctx.text.size())
            THROW("Unexpected EOF");
        if (ctx.text[ctx.pos] != ch)
            THROW("Unexpected character");
        advance(ctx);
    }

    void skipWhitespace(ParsingContext &ctx) {
        while (peep(ctx) >= 0 && isSPACE((char)peep(ctx)))
            skip(ctx);
    }

    void skipString(ParsingContext &ctx, const std::string &str) {
        for (char ch: str)
            skip(ctx, ch);
    }

    std::string readName(ParsingContext &ctx) {
        std::string result;
        int f = peep(ctx);
        if (f >= 0 && isUNCHARnonNUM((char)f)) {
            skip(ctx);
            result += (char)f;
            while (peep(ctx) >= 0 && isUNCHAR((char)peep(ctx)))
                result += skip(ctx);
        }
        return result;
    }

    std::string readUint(ParsingContext &ctx) {
        if (peep(ctx) == '0') {
            skip(ctx);
            return "0";
        }
        std::string result;
        while (peep(ctx) >= 0 && isNUM((char)peep(ctx)))
            result += skip(ctx);
        return result;
    }

    std::vector<std::string> splitIntoLines(const std::string &str) {
        std::vector<std::string> result = {""};
        for (char ch: str) {
            if (ch == '\n')
                result.emplace_back();
            else
                result.back() += ch;
        }
        return result;
    }

    std::string concatenateLines(const std::vector<std::string>& lines) {
        std::string result;
        size_t n = lines.size();
        for (size_t i = 0; i < n; i++) {
            if (i)
                result += '\n';
            result += lines[i];
        }
        return result;
    }

    bool is_relevant_in_tab_cut(size_t PN, size_t I, size_t LN, size_t j, const std::string& line) {
        if (j == 0 && I != 0)
            return false;
        if (!is_space_only(line))
            return true;
        return j + 1 == LN && I + 1 < PN;
    }

    void one_part_update_min_start_wsp_non_empty(const std::string& str, size_t I, size_t PN, size_t& min) {
        std::vector<std::string> lines = splitIntoLines(str);
        size_t LN = lines.size();
        for (size_t j = 0; j < LN; j++) {
            if (is_relevant_in_tab_cut(PN, I, LN, j, lines[j]))
                min = std::min(min, first_nw_char(lines[j]));
        }
    }

    std::string one_part_cut_excess_tab(const std::string& str, size_t I, size_t PN, size_t cut) {
        std::vector<std::string> lines = splitIntoLines(str);
        size_t LN = lines.size();
        for (size_t j = 0; j < LN; j++) {
            if (is_relevant_in_tab_cut(PN, I, LN, j, lines[j]))
                lines[j] = lines[j].substr(cut);
        }
        return concatenateLines(lines);
    }

    Element& add_hidden_element(const std::string& new_el_name, global_elem_set_t& result) {
        if (result.count(new_el_name) != 0)
            THROW("Repated element " + new_el_name);
        TemplaterRegPref& rp = result[new_el_name];
        rp.is_element = 1;
        rp.when_element = std::make_unique<Element>();
        rp.when_element->is_hidden = true;
        return *rp.when_element;
    }

    Element& add_new_element(const std::string& new_el_name, global_elem_set_t& result) {
        if (!is_uname_dotted_sequence(new_el_name))
            THROW("Krabovaya oshibka");
        if (result.count(new_el_name) != 0 && result.at(new_el_name).is_element)
            THROW("Repated element " + new_el_name);
        size_t n = new_el_name.size();
        for (size_t i = 0; i < n; i++) {
            if (new_el_name[i] == '.') {
                std::string pref = new_el_name.substr(0, i);
                result[pref];
            }
        }
        TemplaterRegPref& rp = result[new_el_name];
        rp.is_element = 1;
        rp.when_element = std::make_unique<Element>();
        return *rp.when_element;
    }

    void parse_bare_file(const std::string& filename, const std::string& content, global_elem_set_t& result) {
        Element& el = add_new_element(filename, result);
        std::string txt = clement_lstrip(content);
        rstrip(txt);
        size_t cut = 9999999999999;
        one_part_update_min_start_wsp_non_empty(txt, 0, 1, cut);
        txt = one_part_cut_excess_tab(txt, 0, 1, cut);
        el.parts = {ElementPart{}};
        el.parts[0].when_code.lines = mv(txt);
    }

    /* Type parsing frame */
    struct TPFrame {
        json::JSON& result;

        explicit TPFrame(json::JSON& result_): result(result_){}

        uptr<TPFrame> toMe(bool returned, ParsingContext& ctx) {
            if (!returned) {
                std::string nm = readName(ctx);
                if (nm.empty())
                    THROW("Type specification expected");
                nm = make_uppercase(nm);
                if (nm == "JSON") {
                    result = json::JSON(true);
                    return NULL;
                }
                if (nm != "EL")
                    THROW("Type of argument variable is either JSON or EL(...signature)");
                skip(ctx, '(');
                result.asArray();
                assert(result.isArray());
            }
            skipWhitespace(ctx);
            if (peep(ctx) == ')')
                return NULL;
            result.asArray().emplace_back();
            return std::make_unique<TPFrame>(result.asArray().back());
        }
    };

    json::JSON parse_type(ParsingContext& ctx) {
        json::JSON result;
        std::vector<uptr<TPFrame>> stack;
        stack.push_back(mv(std::make_unique<TPFrame>(result)));
        bool returned = false;
        while (!stack.empty()) {
            uptr<TPFrame> ret = stack.back()->toMe(returned, ctx);
            returned = !(bool)ret;
            if (ret)
                stack.push_back(mv(ret));
            else
                stack.pop_back();
        }
        return result;
    }

    /* From arg name to arg ID */
    typedef std::map<std::string, size_t> arg_name_list_t;

    /* Expression parsing frame */
    struct EPFrame {
        json::JSON& result;

        explicit EPFrame(json::JSON& result_): result(result_){}

        uptr<EPFrame> toMe(bool returned, ParsingContext& ctx, const arg_name_list_t& local_var_names) {
            if (!returned) {
                std::string first = readName(ctx);
                if (first.empty())
                    THROW("Expression should start with 'root' name of global package or local variable");
                if (first == "_")
                    THROW("Expression root can't be _");
                if (local_var_names.count(first) == 1) {
                    result["V"].asInteger() = json::Integer((int64_t)local_var_names.at(first));
                } else {
                    result["V"].asString() = first;
                }
                result["C"].asArray();
            } else {
                skipWhitespace(ctx);
                skip(ctx, ']');
            }
            std::vector<json::JSON>& chain = result["C"].asArray();
            while (true) {
                if (peep(ctx) == '.') {
                    skip(ctx, '.');
                    chain.emplace_back();
                    std::string t;
                    t = readName(ctx);
                    if (!t.empty()) {
                        chain.back() = json::JSON(t);
                        continue;
                    }
                    t = readUint(ctx);
                    if (!t.empty()) {
                        size_t v = std::stoul(t);
                        if (v >= INT64_MAX)
                            THROW("Index is too big");
                        chain.back() = json::JSON((int64_t)v);
                        continue;
                    }
                    THROW("Bad expression after . operator in expression");
                } else if (peep(ctx) == '[') {
                    skip(ctx, '[');
                    skipWhitespace(ctx);
                    chain.emplace_back();
                    return std::make_unique<EPFrame>(chain.back());
                } else
                    return NULL;
            }
        }
    };

    json::JSON parse_expression(ParsingContext& ctx, const arg_name_list_t& local_var_names) {
        json::JSON result;
        std::vector<uptr<EPFrame>> stack;
        stack.push_back(mv(std::make_unique<EPFrame>(result)));
        bool returned = false;
        while (!stack.empty()) {
            uptr<EPFrame> ret = stack.back()->toMe(returned, ctx, local_var_names);
            returned = !(bool)ret;
            if (ret)
                stack.push_back(mv(ret));
            else
                stack.pop_back();
        }
        return result;
    }

    std::string read_code_up_to_mag_block_start(ParsingContext& ctx, const TemplaterSettings& syntax) {
        size_t begin = ctx.pos;
        while (peep(ctx) != EOFVAL && peep(ctx) != syntax.magic_block_start[0]) {
            skip(ctx);
        }
        size_t end = ctx.pos;
        return ctx.text.substr(begin, end - begin);
    }

    void skip_magic_block_start(ParsingContext& ctx, const TemplaterSettings& syntax) {
        skipString(ctx, syntax.magic_block_start);
        skipWhitespace(ctx);
    }

    void skip_magic_block_end(ParsingContext& ctx, const TemplaterSettings& syntax) {
        skipWhitespace(ctx);
        skipString(ctx, syntax.magic_block_end);
    }

    bool isIt_magic_block_end(ParsingContext& ctx, const TemplaterSettings& syntax) {
        return peep(ctx) == syntax.magic_block_end[0];
    }

    /* Element content parsing frame */
    struct ECPFrame {
        enum block_type{
            gone_for_nothing,
            gone_for_for,
            gone_for_ref,
        };
        std::string el_name;
        block_type myself;
        arg_name_list_t local_var_names;
        int& ret_data_int; // Received from the top (and passed down to get for_put LF mode value)
        Element& result;

        block_type stopped_for = gone_for_nothing;
        size_t free_hidden = 0;

        ECPFrame(const std::string& el_name, block_type myself, const arg_name_list_t &local_var_names, int &ret_data_int,
            Element& result)
            : el_name(el_name),
                myself(myself),
                local_var_names(local_var_names),
              ret_data_int(ret_data_int),
              result(result) {
        }

        uptr<ECPFrame> toMe(bool returned, ParsingContext& ctx, const TemplaterSettings& syntax, global_elem_set_t& elem_ns) {
            if (returned) {
                if (stopped_for == gone_for_for) {
                    assert(result.parts.back().type == ElementPart::p_for_put);
                    if (ret_data_int == 1)
                        result.parts.back().when_for_put.line_feed = false;
                    else if (ret_data_int == 2)
                        result.parts.back().when_for_put.line_feed = true;
                    else
                        assert(false);
                } else
                    assert(ret_data_int == 0);
            }
            ret_data_int = 0;
            ya_e_ya_h_i_ya_g_d_o:
            result.parts.emplace_back();
            result.parts.back().when_code.lines = read_code_up_to_mag_block_start(ctx, syntax);
            skip_magic_block_start(ctx, syntax);
            if (isIt_magic_block_end(ctx, syntax)) {
                skip_magic_block_end(ctx, syntax);
                goto ya_e_ya_h_i_ya_g_d_o;
            }
            std::string op = make_uppercase(readName(ctx));
            if (op == "FOR") {
                result.parts.emplace_back();
                result.parts.back().type = ElementPart::p_for_put;
                ElementPart::when_for_put_S& P = result.parts.back().when_for_put;
                skipWhitespace(ctx);
                std::string V1 = readName(ctx);
                if (V1.empty())
                    THROW("Expected variable name");
                skipWhitespace(ctx);
                bool have_colon_and_2 = false;
                std::string V2;
                if (peep(ctx) == ':') {
                    have_colon_and_2 = true;
                    skip(ctx, ':');
                    skipWhitespace(ctx);
                    V2 = readName(ctx);
                    skipWhitespace(ctx);
                }
                op = make_uppercase(readName(ctx));
                if (op != "IN")
                    THROW("Expected IN");
                skipWhitespace(ctx);
                P.ref_over = parse_expression(ctx, local_var_names);
                P.internal_element = el_name + ".~" + std::to_string(free_hidden++);
                Element& newborn = add_hidden_element(P.internal_element, elem_ns);
                arg_name_list_t local_var_names_of_nxt = local_var_names;
                if (V1 != "_") {
                    if (local_var_names_of_nxt.count(V1) != 0)
                        THROW("Repeated local variable");
                    size_t k = local_var_names_of_nxt.size();
                    local_var_names_of_nxt.emplace(V1, k);
                    (have_colon_and_2 ? P.where_key_var : P.where_value_var) = (ssize_t)k;
                }
                if (have_colon_and_2 && V2 != "_") {
                    if (local_var_names_of_nxt.count(V2) != 0)
                        THROW("Repeated local variable");
                    size_t k = local_var_names_of_nxt.size();
                    local_var_names_of_nxt.emplace(V2, k);
                    P.where_value_var = (ssize_t)k;
                }
                skip_magic_block_end(ctx, syntax);
                /* Yep, I am passing this int random data reference, that was actually given to me, I CAN DO THAT TRUST ME */
                stopped_for = gone_for_for;
                return std::make_unique<ECPFrame>(P.internal_element, gone_for_for, local_var_names_of_nxt,
                    ret_data_int, newborn);
            }
            if (op == "REF") {
                result.parts.emplace_back();
                result.parts.back().type = ElementPart::p_ref_put;
                ElementPart::when_ref_put_S& P = result.parts.back().when_ref_put;
                skipWhitespace(ctx);
                std::string Vn = readName(ctx);
                if (Vn.empty() || Vn == "_")
                    THROW("REF: expected variable name");
                skipWhitespace(ctx);
                op = make_uppercase(readName(ctx));
                if (op != "AS")
                    THROW("Expected AS");
                skipWhitespace(ctx);
                P.ref_over = parse_expression(ctx, local_var_names);
                P.internal_element = el_name + ".~" + std::to_string(free_hidden++);
                Element& newborn = add_hidden_element(P.internal_element, elem_ns);
                arg_name_list_t local_var_names_of_nxt = local_var_names;
                size_t k = local_var_names_of_nxt.size();
                local_var_names_of_nxt.emplace(Vn, k);
                skip_magic_block_end(ctx, syntax);
                stopped_for = gone_for_ref;
                return std::make_unique<ECPFrame>(P.internal_element, gone_for_ref, local_var_names_of_nxt,
                    ret_data_int, newborn);
            }
            if (op == "PUT" || op == "P") {
                result.parts.emplace_back();
                result.parts.back().type = ElementPart::p_put;
                ElementPart::when_put_S& P = result.parts.back().when_put;
                skipWhitespace(ctx);
                P.called_element = parse_expression(ctx, local_var_names);
                while (true) {
                    skipWhitespace(ctx);
                    if (isIt_magic_block_end(ctx, syntax)) {
                        skip_magic_block_end(ctx, syntax);
                        break;
                    }
                    P.passed_arguments.push_back(parse_expression(ctx, local_var_names));
                }
                goto ya_e_ya_h_i_ya_g_d_o;
            }
            auto mediocre_operator = [&](const std::string& base_el) -> void {
                result.parts.emplace_back();
                result.parts.back().type = ElementPart::p_put;
                ElementPart::when_put_S& P = result.parts.back().when_put;
                P.called_element["V"] = json::JSON(base_el);
                P.called_element["C"] = json::JSON(json::array);
                skipWhitespace(ctx);
                P.passed_arguments = {parse_expression(ctx, local_var_names)};
                skip_magic_block_end(ctx, syntax);
            };
            if (op == "WRITE" || op == "W") {
                mediocre_operator("str2text");
                goto ya_e_ya_h_i_ya_g_d_o;;
            }
            if (op == "ROUGHINSERT" || op == "RI") {
                mediocre_operator("str2code");
                goto ya_e_ya_h_i_ya_g_d_o;;
            }
            auto prepare_to_depart_parts = [&]() {
                assert(!result.parts.empty());
                if (result.parts[0].type == ElementPart::p_code)
                    result.parts[0].when_code.lines = clement_lstrip(result.parts[0].when_code.lines);
                if (result.parts.back().type == ElementPart::p_code)
                    rstrip(result.parts.back().when_code.lines);
                size_t cut = 999999999999;
                size_t N = result.parts.size();
                for (size_t i = 0; i < N; i++) {
                    if (result.parts[i].type == ElementPart::p_code) {
                        one_part_update_min_start_wsp_non_empty(result.parts[i].when_code.lines, i, N, cut);
                    }
                }
                for (size_t i = 0; i < N; i++) {
                    if (result.parts[i].type == ElementPart::p_code) {
                        result.parts[i].when_code.lines = one_part_cut_excess_tab(result.parts[i].when_code.lines, i, N, cut);
                    }
                }
            };
            if (op == "ENDELDEF") {
                if (myself != gone_for_nothing)
                    THROW("Unexpected ENDELDEF");
                skip_magic_block_end(ctx, syntax);
                prepare_to_depart_parts();
                return NULL;
            }
            if (op == "ENDFOR") {
                if (myself != gone_for_for)
                    THROW("Unexpected ENDFOR");
                skipWhitespace(ctx);
                /* Here I am using ret_data_int to return info about NOLF(1)/LF(2) decision */
                ret_data_int = 2;  // Default is to do LF
                if (!isIt_magic_block_end(ctx, syntax)) {
                    op = make_uppercase(readName(ctx));
                    if (op == "LF") {
                        ret_data_int = 2;
                    } else if (op == "NOLF") {
                        ret_data_int = 1;
                    } else
                        THROW("Expected LF, NOLF or end of magic block");
                }
                skip_magic_block_end(ctx, syntax);
                prepare_to_depart_parts();
                return NULL;
            }
            if (op == "ENDREF") {
                if (myself != gone_for_ref)
                    THROW("Unexpected ENDREF");
                skip_magic_block_end(ctx, syntax);
                prepare_to_depart_parts();
                return NULL;
            }
            THROW("Unknown operator. Expected FOR, REF, PUT, WRITE, ROUGHINSERT, ENDELDEF, ENDFOR, ENDREF");
        }
    };

    void parse_element_content(const std::string& el_name, ParsingContext& ctx, const TemplaterSettings& syntax,
        const arg_name_list_t& local_var_names, Element& result, global_elem_set_t& elem_ns) {
        int random_junk;  // Used onlt for for_put blocks
        std::vector<uptr<ECPFrame>> stack;
        stack.push_back(mv(std::make_unique<ECPFrame>(el_name, ECPFrame::gone_for_nothing, local_var_names, random_junk, result)));
        bool returned = false;
        while (!stack.empty()) {
            uptr<ECPFrame> ret = stack.back()->toMe(returned, ctx, syntax, elem_ns);
            returned = !(bool)ret;
            if (ret)
                stack.push_back(mv(ret));
            else
                stack.pop_back();
        }
    }

    void parse_special_file(const std::string& filename, const std::string& content,
        global_elem_set_t& result, TemplaterSettings& syntax)
    {
        ParsingContext ctx{content};
        while(true) {
            skipWhitespace(ctx);
            if (peep(ctx) == EOFVAL)
                break;
            skip_magic_block_start(ctx, syntax);
            if (make_uppercase(readName(ctx)) != "ELDEF")
                THROW("Expected ELDEF");
            skipWhitespace(ctx);
            std::string elname_postfix = readName(ctx);
            if (elname_postfix == "_")
                THROW("Can't use _ as element name");
            std::string fullname = elname_postfix == "main" ? filename : filename + "." + elname_postfix;
            Element& newborn = add_new_element(fullname, result);
            arg_name_list_t arglist;
            while (true) {
                skipWhitespace(ctx);
                if (isIt_magic_block_end(ctx, syntax))
                    break;
                newborn.arguments.push_back(parse_type(ctx));
                skipWhitespace(ctx);
                std::string argname = readName(ctx);
                if (argname.empty())
                    THROW("Expected argument name");
                if (argname != "_") {
                    if (arglist.count(argname) != 0)
                        THROW("Repeated argument (" + argname + ")");
                    size_t k = arglist.size();
                    arglist[argname] = k;
                }
            }
            skip_magic_block_end(ctx, syntax);
            parse_element_content(fullname, ctx, syntax, arglist, newborn, result);
        }
    }
}
