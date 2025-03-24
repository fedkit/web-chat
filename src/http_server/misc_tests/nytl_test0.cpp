#include <jsonincpp/string_representation.h>
#include <new_york_transit_line/templater.h>
#include <new_york_transit_line/core.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: test assets_dir");
        exit(1);
    }

    std::string dir_path = argv[1];
    nytl::Templater templater(nytl::TemplaterSettings{nytl::TemplaterDetourRules{dir_path}});
    templater.update();
    nytl::debug_print_templater(templater);
    json::JSON cba;
    cba["boba"] = json::JSON("<>");
    cba["arr"][0] = json::JSON("zero");
    cba["arr"][1] = json::JSON("one");
    cba["arr"][2] = json::JSON("two");
    cba["k"] = json::JSON("arr");
    cba["i"] = json::JSON(1l);
    // printf("DEBUG WAS: %p\n", &cba["boba"].g());
    // printf("%s\n", json::generate_str(cba["boba"].g(), json::print_compact).c_str());
    // return 0;
    std::string answer2 = templater.render("test", {&cba});
    printf("%s\n<a><f><t><e><r><><l><f>\n", answer2.c_str());

    return 0;
}
