#include "static_asset_manager.h"
#include "../os_utils.h"
#include "../baza_inter.h"
#include <memory>
#include <utility>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

namespace een9 {
    std::vector<std::string> detour_over_regular_folder(const std::string& path) {
        std::vector<std::string> result;
        int ret;

        std::vector<std::string> todo;
        todo.emplace_back();
        while (!todo.empty()) {
            std::string cur = std::move(todo.back());
            todo.pop_back();
            std::string path_to_cur_ent = path + "/" + cur;
            struct stat info;
            ret = stat(path_to_cur_ent.c_str(), &info);
            ASSERT_on_iret(ret, "stat(\"" + cur + "\")");
            if (S_ISDIR(info.st_mode)) {
                DIR* D = opendir(path_to_cur_ent.c_str());
                struct Guard1{ DIR*& D; ~Guard1(){ closedir(D); } } g1{D};
                ASSERT(D != NULL, prettyprint_errno("opendir(\"" + cur +"\")"));
                cur += "/";
                while (true) {
                    errno = 0;
                    struct dirent* Dent = readdir(D);
                    if (Dent == NULL) {
                        if (errno == 0)
                            break;
                        THROW_on_errno("dirent in \"" + cur + "\"");
                    }
                    std::string child_entry = Dent->d_name;
                    if (child_entry != "." && child_entry != "..")
                        todo.push_back(cur + child_entry);
                }
            } else if (S_ISREG(info.st_mode)) {
                result.push_back(cur);
            } else {
                THROW("unknown fs entry type \"" + cur + "\"");
            }
        }
        return result;
    }

    void updateStaticAssetManager(StaticAssetManager& sam) {
        sam.url_to_asset.clear();
        for (const StaticAssetManagerRule& dir_rule: sam.rules) {
            std::vector<std::string> c_files = detour_over_regular_folder(dir_rule.directory);
            for (const std::string& file: c_files) {
                for (const StaticAssetManagerRulePostfixFilter& ext: dir_rule.postfix_rules_type_assign) {
                    if (endsWith(file, ext.required_postfix)) {
                        /* Found it! */
                        StaticAsset etot{ext.assigned_type, };
                        readFile(dir_rule.directory + "/" + file, etot.content);
                        sam.url_to_asset[dir_rule.url_prefix + file] = etot;
                        break;
                    }
                }
            }
        }
    }

    int StaticAssetManagerSlaveModule::get_asset(const std::string& url, StaticAsset& ret) {
        RwlockReadGuard lg(mut);
        if (sam.url_to_asset.count(url) == 0)
            return -1;
        ret = sam.url_to_asset[url];
        return 0;
    }

    void StaticAssetManagerSlaveModule::update() {
        RwlockWriteGuard lg(mut);
        updateStaticAssetManager(sam);
    }

    void StaticAssetManagerSlaveModule::update(std::vector<StaticAssetManagerRule> new_rules) {
        RwlockWriteGuard lg(mut);
        sam.rules = std::move(new_rules);
        updateStaticAssetManager(sam);
    }
}
