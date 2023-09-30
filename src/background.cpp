#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <future>
#include <unistd.h> //linux only library

#include <dpp/dpp.h>

#include "readFile.h"
#include "ping.h"

void backgroundThread(std::vector<std::string> envData){

    dpp::cluster bot(envData[0]);
 
    bot.on_log(dpp::utility::cout_logger());
 
    bot.on_ready([&bot, &envData](const dpp::ready_t& event){
        for(int i = 0; i < 10; i++){
            //sleep for set time in seconds
            // sleep(10);

            //we use promises to get data out of the callback functions from bot.request
            std::promise<int> promiseStatus;
            std::promise<int> promiseHome;
            //we use futures to wait for our promises to resolve
            std::future<int> futureStatus = promiseStatus.get_future();
            std::future<int> futureHome = promiseHome.get_future();

            //bot.request is an asynchronous function
            // check /status
            bot.request("https://mirror.clarkson.edu/status", dpp::m_get, [&promiseStatus](const dpp::http_request_completion_t& request){
                promiseStatus.set_value(request.status);
            });
            //check /home
            bot.request("https://mirror.clarkson.edu/home", dpp::m_get, [&promiseHome](const dpp::http_request_completion_t& request){
                promiseHome.set_value(request.status);
            });

            //asycronosly ping mirror while http requests are running
            std::future<std::pair<bool, std::string>> futurePingObj = std::async(ping, "mirror.clarkson.edu");

            //wait for futures to complete before accessing the result
            std::pair<bool, std::string> pingObj = futurePingObj.get(); //blocking
            int statusInt = futureStatus.get(); //blocking
            int homeInt = futureHome.get(); //blocking

            std::cout << pingObj.first << std::endl;
            std::cout << statusInt << std::endl; 
            std::cout << homeInt << std::endl; 

            std::vector<std::string> channels = readFile("channels.txt");

            for(int i = 0; i < channels.size(); i++){
                // create a message
                long long channel_id = std::stoll(channels[i]);
                dpp::message message(dpp::snowflake(channel_id), pingObj.second);

                //send message
                bot.message_create(message, [&bot](const dpp::confirmation_callback_t& callback){
                    if (callback.is_error()){
                        //log error
                        std::cout << "failed to send message" << std::endl;
                    }
                });
            }

        }
        
    });
 
    bot.start(dpp::st_wait);
}