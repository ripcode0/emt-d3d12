#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <string>
#include <span>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define EMT_DEPS_VERSION_STRING "v" STR(EMT_DEPS_VERSION_MAJOR) "." STR(EMT_DEPS_VERSION_MINOR) "." STR(EMT_DEPS_VERSION_PATCH)
#define EMT_DEPS_VERSION_CODE ((EMT_DEPS_VERSION_MAJOR * 10000) + (EMT_DEPS_VERSION_MINOR * 100) + EMT_DEPS_VERSION_PATCH)

namespace fs = std::filesystem;
using parser_map = std::unordered_map<std::string, std::string>;

parser_map args_parser(int args, char* argv[]){
    
    parser_map flags;
    
    for(int i = 0; i < args; ++i){
        std::string arg = argv[i];
        if(arg.starts_with("--")){
            std::string key = arg.substr(2);
            std::string value = "true";

            if(i + 1 < args){
                std::string next_arg = argv[i + 1];
                //std::cout << "next : " << next_arg << std::endl;

                if(!next_arg.starts_with("--")){
                    value = next_arg;
                    ++i;
                }else{
                    value = "none";
                    ++i;
                }
                
                //std::cout << "value : " << value << std::endl;
            } else{
                value = "none";
            }

            flags[key] = value;
            //std::cout << "key : " << flags[key] << std::endl;

        }        

    }

    return flags;
}

void filer_flags(parser_map& flags){
    
    parser_map::iterator it;
    for(it = flags.begin(); it != flags.end();){
        if(it->second == "true"){
            it = flags.erase(it);
        } else {
            it++;
        }
    }
}

void filter_flags_operator(parser_map& flags ,const std::span<const char*> operators){
    for(auto it = flags.begin(); it != flags.end();){
        bool found = false;
        for(size_t i = 0; i < operators.size(); ++i){
            if(it->first == operators[i]){
                found = true;
                break;
            }
        }
        if(!found){
            it = flags.erase(it);
        }else{
            ++it;
        }
    }
}

void print_aligned_flags(const std::unordered_map<std::string, std::string>& flags) {
    size_t max_key_len = 0;
    for (const auto& [key, _] : flags) {
        if (key.length() > max_key_len) {
            max_key_len = key.length();
        }
    }

    std::cout << "============== final enable flags ===================" << std::endl;
    for (const auto& [key, value] : flags) {
        std::cout << "key : " << std::left << std::setw(static_cast<int>(max_key_len)) << key
                  << " | value : " << value << std::endl;
    }
}

int main(int args, char* argv[])
{
    auto flags = args_parser(args, argv);

    filer_flags(flags);

    const char* operators[] = {
        "git" , "prefix", "config"
    };

    filter_flags_operator(flags, operators);

    print_aligned_flags(flags);

    std::cout << "version : " << EMT_DEPS_VERSION_STRING << std::endl;

    return 0;
}