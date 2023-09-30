#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "readFile.h"

std::vector<std::string> readFile(std::string filename){
    std::vector<std::string> output;
    std::string line;
    std::ifstream file(filename);
    while(getline(file, line)){
        output.push_back(line);
    }
    file.close();
    return output;

}