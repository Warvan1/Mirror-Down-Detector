#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "readFile.h"
#include "bot.h"
#include "background.h"
 
int main() {
    //read env file
    const std::vector<std::string> envData = readFile("../.env");
    
    std::thread t1(botThread, envData);
    std::thread t2(backgroundThread, envData);

    t1.join();
    t2.join();
}