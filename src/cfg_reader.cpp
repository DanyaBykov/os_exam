// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "cfg_reader.h"

const std::string DELIMS = " \n\t\r\f=";
const std::vector<std::string> CFG_KEYS = {"abs_err","rel_err","x_end","x_start","y_start","y_end","init_steps_x","init_steps_y","max_iter"};

std::vector<std::string> divide_by_delims (const std::string& origin)
{
    std::vector<std::string> dest;
    auto first = std::cbegin(origin);
    while (first != std::cend(origin))
    {
        const auto second = std::find_first_of(first, std::cend(origin), std::cbegin(DELIMS), std::cend(DELIMS));
        if (first != second)
        {
            dest.emplace_back(first, second);
        }
        if (second == std::cend(origin)) break;
        first = std::next(second);
    }
    return dest;
}

std::map<std::string, float> input_processing (std::vector<std::string> input)
{
    std::map<std::string, float> result;
    auto first = std::cbegin(input);
    while (first != std::cend(input))
    {
        if (std::find(CFG_KEYS.begin(), CFG_KEYS.end(), *first) != CFG_KEYS.end()) {
            auto second = std::next(first);
            if (second != input.end()) { // Ensure second exists
                try {
                    result[*first] = std::stof(*second);
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing float from: " << *second << "\n";
                }
                first = second;
            }
        }
        ++first;
    }
    return result;
}

std::vector<std::string> read_file(std::string file_name)
{
    std::ifstream filestream(file_name);
    std::vector<std::string> data;
    auto ss = std::ostringstream{};
    ss << filestream.rdbuf();
    auto filedata = ss.str();
    data = divide_by_delims(filedata);
    return data;
}

// int main(int argc, char* argv[])
// {
//     std::string file_name = "../cfg_files/func1.cfg";
//     std::vector<std::string> data = read_file(file_name);
//     std::map<std::string, float> res = input_processing(data);
//     for (auto it = res.begin(); it != res.end(); ++it) {
//         std::cout << it->first << ": " << it->second << "\n";
//     }
//     return  0;
// }