#include "templater.h"
#include <sys/stat.h>
#include <dirent.h>
#include "alotalot.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "core.h"

namespace nytl {
    /* Throws std::runtime_exception on incorrect settings */
    void check_settings(const TemplaterSettings& settings) {
#define lmao_serving_kid_with_identity_issues throw std::runtime_error("What was wrong with {% %} ????")
        if (settings.magic_block_start.empty() || settings.magic_block_end.empty())
            lmao_serving_kid_with_identity_issues;
        char incode = settings.magic_block_start[0];
        if (isSPACE(incode) || isALPHA(incode) || isNUM(incode))
            lmao_serving_kid_with_identity_issues;
        char ender = settings.magic_block_end[0];
        if (isUNCHAR(ender) || ender == ':' || isSPACE(ender) || ender == '[' || ender == ']' || ender == '.')
            lmao_serving_kid_with_identity_issues;
    }

    Templater::Templater(TemplaterSettings settings): settings(std::move(settings)) {
        check_settings(this->settings);
    }

    struct InterestingFile {
        std::string path;
        std::string dot_name;
        bool special_syntax_applied;
    };

    std::vector<InterestingFile> indexing_detour(const TemplaterDetourRules& rules) {
        std::vector<InterestingFile> result;
        int ret;

        std::vector<std::string> todo;
        todo.emplace_back();
        while (!todo.empty()) {
            std::string cur = mv(todo.back());
            todo.pop_back();
            std::string path_to_cur_dir = rules.root_dir_path + "/" + cur;
            DIR* D = opendir(path_to_cur_dir.c_str());
            struct Guard1{ DIR*& D; ~Guard1(){ closedir(D); } } g1{D};
            ASSERT(D != NULL, prettyprint_errno("opendir(\"" + cur +"\")"));
            while (true) {
                errno = 0;
                struct dirent* Dent = readdir(D);
                if (Dent == NULL) {
                    if (errno == 0)
                        break;
                    THROW_on_errno("dirent in \"" + cur + "\"");
                }
                std::string child_entry = Dent->d_name;
                if (child_entry == "." || child_entry == "..")
                    continue;
                std::string path_to_cur_child = path_to_cur_dir + "/" + child_entry;
                struct stat info;
                ret = stat(path_to_cur_child.c_str(), &info);
                ASSERT_on_iret(ret, "stat(" + path_to_cur_child + ")");
                if (S_ISDIR(info.st_mode)) {
                    if (isUname(child_entry))
                        todo.push_back(cur.empty() ? child_entry : cur + "/" + child_entry);
                } else if (S_ISREG(info.st_mode)) {
                    auto replace_sep = [](const std::string& slashed) -> std::string {
                        std::string dotted;
                        dotted.reserve(slashed.size());
                        for (char ch: slashed) {
                            if (ch == '/')
                                dotted += '.';
                            else
                                dotted += ch;
                        }
                        return dotted;
                    };
                    auto np_reg_categ_result = [&](const std::string& no_postfix, bool applied) {
                        if (isUname(no_postfix))
                            result.push_back({path_to_cur_child, replace_sep(cur.empty() ? no_postfix : cur + "/" + no_postfix), applied});
                    };
                    if (endsIn(child_entry, rules.postfix_rule_for_element_cont)) {
                        np_reg_categ_result(throwout_postfix(child_entry, rules.postfix_rule_for_element_cont.size()), true);
                    } else if (endsIn(child_entry, rules.postfix_rule_for_static_files)) {
                        np_reg_categ_result(throwout_postfix(child_entry, rules.postfix_rule_for_static_files.size()), false);
                    }
                } else {
                    THROW("unknown fs entry type \"" + cur + "\"");
                }
            }
        }
        return result;
    }

    std::string readFile(const std::string& path) {
        std::string result;
        int ret;
        int fd = open(path.c_str(), O_RDONLY);
        ASSERT_on_iret(fd, "Opening \"" + path + "\"");
        char buf[2048];
        while ((ret = (int)read(fd, buf, 2048)) > 0) {
            size_t oldN = result.size();
            result.resize(oldN + ret);
            memcpy(result.data() + oldN, buf, ret);
        }
        if (ret < 0) {
            close(fd);
            THROW("reading file");
        }
        return result;
    }

    TemplaterRegPref gen_base_element() {
        Element* e = new Element{{json::JSON(true)}, true, false, {}};
        return {1, std::unique_ptr<Element>(e)};
    }

    void Templater::update() {
        elements[""] = TemplaterRegPref{0, NULL};
        elements["jsinsert"] = gen_base_element();
        elements["jesc"] = gen_base_element();
        elements["str2text"] = gen_base_element();
        elements["str2code"] = gen_base_element();
        std::vector<InterestingFile> intersting_files = indexing_detour(settings.det);
        for (const InterestingFile& file: intersting_files) {
            std::string content = readFile(file.path);
            if (file.special_syntax_applied) {
                parse_special_file(file.dot_name, content, elements, settings);
            } else {
                parse_bare_file(file.dot_name, content, elements);
            }
        }
    }

    /* Still can throw some stuff derived from std::exception (like bad alloc) */
    std::string Templater::render(const std::string& element, const std::vector<const json::JSON*> &arguments) const {
        if (!is_uname_dotted_sequence(element))
            THROW("Incorrect entry element name");
        return rendering_core(element, arguments, elements, settings.escape);
    }
}
