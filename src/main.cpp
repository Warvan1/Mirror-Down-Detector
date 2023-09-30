#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <future>
#include <unistd.h> //linux only library
#include <thread>

#include <dpp/dpp.h>

#include "readFile.h"
#include "ping.h"

void botThread(std::vector<std::string> envData){

    dpp::cluster bot(envData[0]);
 
    bot.on_log(dpp::utility::cout_logger());
 
    bot.on_slashcommand([](const dpp::slashcommand_t& event){
        //command used to register channels to recieve down-detection alerts.
        if (event.command.get_command_name() == "watch-mirror") {
            //channel id
            uint64_t e_channel_id = event.command.channel_id;
            //user object
            dpp::user e_user = event.command.get_issuing_user();
            //guild object
            dpp::guild e_guild = event.command.get_guild();
            //member object
            dpp::guild_member e_member = e_guild.members.find(e_user.id)->second;
            //members permissions
            dpp::permission e_permissions = e_guild.base_permissions(e_member);
            //bool representing mod status of the user
            bool is_mod = e_permissions.can(dpp::p_kick_members, dpp::p_ban_members);

            if(is_mod == true){
                //read the channel file into a list of channels
                std::vector<std::string> channels = readFile("channels.txt");

                //check file to make sure that channel isnt there already
                bool channelInFile = false;
                for(int i = 0; i < channels.size(); i++){
                    if (std::to_string(e_channel_id) == channels[i]){
                        channelInFile = true;
                    }
                }

                //add channel to file if not already there
                if(channelInFile == false){
                    channels.push_back(std::to_string(e_channel_id));
                    std::cout << channels[0] << std::endl;
                    std::ofstream channelFile;
                    channelFile.open("channels.txt");
                    for(int i = 0; i < channels.size(); i++){
                        channelFile << channels[i] << std::endl;
                    }
                    channelFile.close();
                    event.reply("this channel will now recieve down-detection messages.");
                }
                else{
                    event.reply("this channel is already recieving down-detection messages.");
                }
            }
            else{
                event.reply("You dont have permission to use that command.");
            }
        }
    });
 
    bot.on_ready([&bot, &envData](const dpp::ready_t& event){
        //register slash commands globably using run_once so that it only runs once
        if(envData[1] == "global"){
            if (dpp::run_once<struct register_bot_commands>()){
                bot.global_command_create(
                    dpp::slashcommand("watch-mirror", "designates this channel to recieve mirror downtime alerts", bot.me.id)
                );
            }
        }
        //register slash commands for a specific server for testing
        else if (envData[1] == "test" && envData.size() >= 3){
            long long guild_id = std::stoll(envData[2]); //stoll is string to long long
            bot.guild_command_create(
                dpp::slashcommand("watch-mirror", "designates this channel to recieve mirror downtime alerts \"test\"", bot.me.id)
            , guild_id);
        }
        else{
            throw std::invalid_argument("incorrect env file");
        }
        
    });
 
    bot.start(dpp::st_wait);
}

void backgroundThread(std::vector<std::string> envData){

    dpp::cluster bot(envData[0]);
 
    bot.on_log(dpp::utility::cout_logger());
 
    bot.on_ready([&bot, &envData](const dpp::ready_t& event){
        for(int i = 0; i < 10; i++){
            //sleep for set time in seconds
            sleep(10);

            //we use promises to get data out of the callback functions from bot.request
            std::promise<int> promiseStatus;
            std::promise<int> promiseHome;
            //we use futures to wait for our promises to resolve
            std::future<int> futureStatus = promiseStatus.get_future();
            std::future<int> futureHome = promiseHome.get_future();

            //bot.request is an asynchronous function however its callback isnt run asynchronously 
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

            // // create a message
            dpp::message message(dpp::snowflake(473179069673111554), pingObj.second);

            //send message
            bot.message_create(message, [&bot](const dpp::confirmation_callback_t& callback){
                if (callback.is_error()){
                    //log error
                    std::cout << "failed to send message" << std::endl;
                }
            });

        }
        
    });
 
    bot.start(dpp::st_wait);
}
 
int main() {
    //read env file
    const std::vector<std::string> envData = readFile("../.env");
    
    std::thread t1(botThread, envData);
    std::thread t2(backgroundThread, envData);

    t1.join();
    t2.join();
}