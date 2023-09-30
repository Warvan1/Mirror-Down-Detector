#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <future>
#include <unistd.h> //linux only library
#include <ctime>

#include <dpp/dpp.h>

#include "readFile.h"
#include "ping.h"

dpp::embed createErrorEmbed(std::vector<uint16_t> errorCodes, std::string ping){
    //create embed object
    dpp::embed embed;

    if(errorCodes[0] == 200 && errorCodes[1] == 200 && errorCodes[2] == 1){
        //everything is back to running well
        embed.set_title("Mirror Is Up!!!!");
    }
    else{
        //something is down
        embed.set_title("Mirror Is Down!!!!");
    }

    //create a field for /status page
    embed.add_field("mirror.clarkson.edu/status", std::string("HTTP status: ") + std::to_string(errorCodes[0]));
    //create a field for /home page
    embed.add_field("mirror.clarkson.edu/home", std::string("HTTP status: ") + std::to_string(errorCodes[1]));
    //set the discription equal to the ping text
    embed.set_description(ping);
    //create footer
    dpp::embed_footer ef;
    embed.set_footer(ef.set_text("COSI Mirror Down Detection").set_icon("https://avatars.githubusercontent.com/u/5634011?s=48&v=4"));
    return embed;
}

void backgroundThread(std::vector<std::string> envData){

    dpp::cluster bot(envData[0]);
 
    bot.on_log(dpp::utility::cout_logger());
 
    bot.on_ready([&bot, &envData](const dpp::ready_t& event){

        std::vector<uint16_t> errorCodes {200, 200, 1};

        while(true){
            //sleep for set time in seconds
            sleep(10);

            //we use promises to get data out of the callback functions from bot.request
            std::promise<uint16_t> promiseStatus;
            std::promise<uint16_t> promiseHome;
            //we use futures to wait for our promises to resolve
            std::future<uint16_t> futureStatus = promiseStatus.get_future();
            std::future<uint16_t> futureHome = promiseHome.get_future();

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
            uint16_t statusInt = futureStatus.get(); //blocking
            uint16_t homeInt = futureHome.get(); //blocking

            //create currient error codes object
            std::vector<uint16_t> currientErrorCodes = {statusInt, homeInt, pingObj.first};

            //compare currient status codes to errorCodes vector
            bool sendMessage = false;
            for(int i = 0; i < errorCodes.size(); i++){
                if(errorCodes[i] != currientErrorCodes[i]){
                    sendMessage = true;
                }
            }

            //send message (and update errorCodes) when there is a change
            if(sendMessage == true){
                //update error codes
                errorCodes = currientErrorCodes;
                //read channel file
                std::vector<std::string> channels = readFile("channels.txt");

                //send messages to each channel in file
                for(int i = 0; i < channels.size(); i++){
                    //currient timestamp
                    std::time_t timestamp = std::time(nullptr);
                    // convet channel id to long long
                    long long channel_id = std::stoll(channels[i]);
                    //create a message object
                    dpp::message message(dpp::snowflake(channel_id), std::string("<t:") + std::to_string(timestamp) + ":F>");
                    //create an embed object
                    dpp::embed embed = createErrorEmbed(currientErrorCodes, pingObj.second);
                    //add embed object to message object
                    message.add_embed(embed);

                    //send message
                    bot.message_create(message, [&bot](const dpp::confirmation_callback_t& callback){
                        if (callback.is_error()){
                            //log error
                            std::cout << "failed to send message" << std::endl;
                        }
                    });
                }
            }
            
        }
    });
    bot.start(dpp::st_wait);
}