#ifndef IU9_CA_WEB_CHAT_LIB_LOCALIZATOR_H
#define IU9_CA_WEB_CHAT_LIB_LOCALIZATOR_H

#include <jsonincpp/jsonobj.h>

namespace iu9cawebchat {
    /* '*' -> ''; X -> X */
    std::string languageRangeSimpler(const std::string& a);

    struct LocalizatorSettings {
        std::string lang_dir;
        std::vector<std::string> whitelist;
        std::vector<std::string> force_order;
    };

    /* There is no need to put http Content-Language response value into json file. When is is in the name */
    struct LanguageFile {
        std::string languagerange;
        json::JSON content;
    };

    /* Localizator uses libjsonincpp internally, and thus can't be read by two treads simultaneously */
    struct Localizator {
        LocalizatorSettings settings;
        std::vector<LanguageFile> files;
        std::map<std::string, size_t> prefix_to_file;

        /* Throws std::exception if something goes wrong */
        explicit Localizator(const LocalizatorSettings& settings);

        /* Returns a reference to object inside Localizator */
        const LanguageFile& get_right_locale(const std::vector<std::string>& preferred_langs);
    };
}

#endif
