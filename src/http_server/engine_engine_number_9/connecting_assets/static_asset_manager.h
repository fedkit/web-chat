#ifndef ENGINE_ENGINE_NUMBER_9_CONNECTING_ASSETS_STATIC_ASSET_MANAGER_H
#define ENGINE_ENGINE_NUMBER_9_CONNECTING_ASSETS_STATIC_ASSET_MANAGER_H

#include <vector>
#include <string>
#include <map>
#include "../thread_synchronization.h"

namespace een9 {
    struct StaticAssetManagerRulePostfixFilter {
        std::string required_postfix;
        std::string assigned_type;
    };

    struct StaticAssetManagerRule {
        std::string directory;
        std::string url_prefix;  // Better end with /
        /* These are rules that filter name ending. First is a required name postfix, Second is a type of document
         * that gets assigned to matching files
         */
        std::vector<StaticAssetManagerRulePostfixFilter> postfix_rules_type_assign;
    };

    struct StaticAsset {
        std::string type;
        std::string content;
    };

    struct StaticAssetManager {
        std::vector<StaticAssetManagerRule> rules;

        std::map<std::string, StaticAsset> url_to_asset;
    };

    void updateStaticAssetManager(StaticAssetManager& sam);

    struct StaticAssetManagerSlaveModule {
        RwlockObj mut;
        StaticAssetManager sam;

        /* Returns newgative on failure. Still can throw execptions derived from std::execption */
        int get_asset(const std::string& url, StaticAsset& ret);

        void update();

        void update(std::vector<StaticAssetManagerRule> new_rules);
    };
}

#endif
