#include "localizator.h"

#include <jsonincpp/string_representation.h>
#include <sys/stat.h>
#include <dirent.h>
#include <engine_engine_number_9/os_utils.h>
#include <engine_engine_number_9/baza_throw.h>
#include <assert.h>

namespace iu9cawebchat {
    std::string languageRangeSimpler(const std::string& a) {
        return a == "*" ? "" : a;
    }

    // I won't use iterators. c plus plus IS a scripting language and I do not want to mess with iterators
    std::vector<std::string> languageRangeGetPrefixes(const std::string& lr) {
        if (lr.empty())
            return {""};
        std::vector<std::string> result = {"", ""};
        for (size_t i = 0; i < lr.size(); i++) {
            if (lr[i] == '-')
                result.push_back(result.back());
            result.back() += lr[i];

        }
        return result;
    }

    bool isInWhitelist(const std::string& lr, const std::vector<std::string>& whitelist) {
        for (const std::string& prefix : languageRangeGetPrefixes(lr))
            for (const std::string& nicePrefix: whitelist)
                if (prefix == nicePrefix)
                    return true;
        return false;
    }

    std::vector<LanguageFile> collect_lang_dir_content(const std::string& lang_dir,
        const std::vector<std::string>& whitelist) {

        std::vector<LanguageFile> result;
        errno = 0;
        DIR* D = opendir(lang_dir.c_str());
        struct Guard1{ DIR*& D; ~Guard1(){ closedir(D); } } g1{D};
        if (!D)
            een9_THROW_on_errno("opendir (" + lang_dir + ")");
        while (true) {
            errno = 0;
            struct dirent* Dent = readdir(D);
            if (Dent == NULL) {
                if (errno == 0)
                    break;
                een9_THROW_on_errno("dirent");
            }
            std::string entry = Dent->d_name;
            if (entry == "." || entry == "..")
                continue;
            std::string filename = lang_dir + "/" + entry;
            struct stat info;
            int ret = stat(filename.c_str(), &info);
            een9_ASSERT_on_iret(ret, "stat(" + filename + ")");
            if (!S_ISREG(info.st_mode))
                continue;
            const std::string postfix = ".lang.json";
            if (!een9::endsWith(entry, postfix))
                continue;
            std::string lang_antirange = entry.substr(0, entry.size() - postfix.size());
            if (!isInWhitelist(lang_antirange, whitelist))
                continue;
            std::string content;
            een9::readFile(filename, content);

            result.emplace_back();
            result.back().languagerange = languageRangeSimpler(lang_antirange);
            result.back().content = json::parse_str_flawless(content);
        }
        return result;
    }


    Localizator::Localizator(const LocalizatorSettings &settings) : settings(settings) {
        /* First - length of the longest prefix that was found so far (in force_order)
         * Second - index in force_order that was assigned to this thingy
         */

        files = collect_lang_dir_content(settings.lang_dir, settings.whitelist);
        size_t n = files.size();
#define redundantFileMsg "Redundant localization file"
        for (size_t i = 0; i < n; i++) {
            for (size_t j = i + 1; j < n; j++) {
                std::string A = files[i].languagerange;
                std::string B = files[j].languagerange;
                for (std::string& pa: languageRangeGetPrefixes(A))
                    if (pa == B)
                        een9_THROW(redundantFileMsg);
                for (std::string& pb: languageRangeGetPrefixes(B))
                    if (pb == A)
                        een9_THROW(redundantFileMsg);
            }
        }
        std::map<std::string, std::vector<std::size_t>> pref_to_files;
        for (size_t k = 0; k < n; k++) {
            for (const std::string& prefix: languageRangeGetPrefixes(files[k].languagerange)) {
                pref_to_files[prefix].push_back(k);
            }
        }
        std::vector<std::pair<size_t, size_t>> assignment;
        constexpr size_t inf_bad_order = 999999999;
        assignment.assign(n, {0, inf_bad_order});
        if (settings.force_order.size() >= inf_bad_order - 2)
            een9_THROW("o_O");
        for (ssize_t i = 0; i < settings.force_order.size(); i++) {
            const std::string& ip = settings.force_order[i];
            if (pref_to_files.count(ip) != 1)
                een9_THROW("force-order list contains entries that match no files (" + ip + ")");
            for (size_t k: pref_to_files.at(ip)) {
                if (assignment[k].first <= ip.size()) {
                    assignment[k].first = ip.size();
                    assignment[k].second = i;
                }
            }
        }
        for (auto& p: pref_to_files) {
            const std::vector<size_t>& candidates = p.second;
            assert(!candidates.empty());
            size_t bestSoFar = candidates[0];
            size_t f = inf_bad_order;
            for (size_t k: candidates) {
                if (assignment[k].second <= f) {
                    f = assignment[k].second;
                    bestSoFar = k;
                }
            }
            prefix_to_file[p.first] = bestSoFar;
        }
        if (prefix_to_file.count("") != 1)
            een9_THROW("No locales were provided");
        // todo: remove DEBUG
        // for (size_t k = 0; k < n; k++) {
            // printf("%s has priority %lu\n", files[k].languagerange.c_str(), assignment[k].second);
        // }
        // printf("==============\n");
        // for (const auto& p : prefix_to_file) {
            // printf("%s  ->  %s\n", p.first.c_str(), files[p.second].languagerange.c_str());
        // }
    }

    const LanguageFile& Localizator::get_right_locale(const std::vector<std::string> &preferred_langs) {
        for (const std::string& lr: preferred_langs) {
            if (prefix_to_file.count(lr) == 1)
                return files[prefix_to_file.at(lr)];
        }
        return files[prefix_to_file.at("")];
    }
}
