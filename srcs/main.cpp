#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <string>
#include <span>

namespace fs = std::filesystem;
using parser_map = std::unordered_map<std::string, std::string>;

parser_map args_parser(int args, char* argv[]){
    std::cout << fs::path::root_directory << std::endl;
    parser_map flags;
    printf("args : %d\n", args);
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
                }
                
                //std::cout << "value : " << value << std::endl;
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

int main(int args, char* argv[])
{
    auto flags = args_parser(args, argv);

    filer_flags(flags);

    const char* operators[] = {
        {"git"}, {"prefix"}
    };

    filter_flags_operator(flags, operators);

    std::cout << "============== final enable flags ===================" << std::endl;
    for(const auto& [key, value] : flags){
        
        std::cout << "key : " << key << " value : " << value << std::endl;
    }

    

    return 0;
}