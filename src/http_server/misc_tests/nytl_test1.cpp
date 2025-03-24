#include <jsonincpp/string_representation.h>
#include <new_york_transit_line/templater.h>
#include <new_york_transit_line/core.h>
#include <engine_engine_number_9/os_utils.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: test assets_dir config_file");
        exit(1);
    }

    // std::string dir_path = "./src/http_server/misc_tests/HypertextPages";
    std::string dir_path = "/home/gregory/cpp_projects/iu9-ca-web-chat/assets/HypertextPages";
    nytl::Templater templater(nytl::TemplaterSettings{nytl::TemplaterDetourRules{dir_path}});
    templater.update();

    json::JSON userprofile;
    userprofile["uid"].asInteger() = json::Integer(0l);
    userprofile["name"].asString() = "radasdasdasdadsdasd";
    userprofile["nickname"].asString() = "root";
    userprofile["bio"].asString() = "Your mother";
    json::JSON errors;
    errors = json::JSON(json::array);
    std::string answer2 = templater.render("err-404", {});
    printf("%s\n<a><f><t><e><r><><l><f>\n", answer2.c_str());

    return 0;
}
