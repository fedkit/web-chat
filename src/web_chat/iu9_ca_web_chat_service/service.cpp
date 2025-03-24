#include <engine_engine_number_9/baza_throw.h>
#include <engine_engine_number_9/os_utils.h>
#include <assert.h>
#include <jsonincpp/string_representation.h>
#include <iu9_ca_web_chat_lib/actions.h>
#include <stdexcept>

void usage(char* argv0) {
    printf("Usage: %s <file with settings> <run|initialize>\n", argv0);
    exit(1);
}

int main(int argc, char** argv){
    try {
        if (argc < 1)
            return 111;
        if (argc != 1 + 2)
            usage(argv[0]);
        std::string config_file = argv[1];
        if (!een9::isRegularFile(config_file) || !een9::endsWith(config_file, ".json")) {
            printf("\"%s\" is not a json file\n", argv[1]);
            usage(argv[0]);
        }
        std::string cmd = argv[2];
        std::string config_text;
        een9::readFile(config_file, config_text);
        const json::JSON config = json::parse_str_flawless(config_text);

        if (cmd == "initialize") {
            const char* ROOT_PW = getenv("ROOT_PW");
            een9_ASSERT(ROOT_PW, "No root password specified."
                                 "Assign desired root password value to environment variable ROOT_PW");
            std::string root_pw = ROOT_PW;
            iu9cawebchat::initialize_website(config, root_pw);
        } else if (cmd == "run") {
            iu9cawebchat::run_website(config);
        } else if (cmd == "version") {
            printf("IU9 Collarbone Annihilation Web Chat (service) V 1.0\n");
        } else
            een9_THROW("unknown action (known are 'run', 'initialize')");
    } catch (std::exception& e) {
        printf("System failure\n%s\n", e.what());
    }
    return 0;
}
