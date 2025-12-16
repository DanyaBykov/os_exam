//
// Created by ivan2 on 2/25/2025.
//

#ifndef CFG_READER_H
#define CFG_READER_H

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <map>

std::vector<std::string> divide_by_delims (const std::string& origin);
std::map<std::string, float> input_processing (std::vector<std::string> input);
std::vector<std::string> read_file(std::string file_name);

#endif //CFG_READER_H
