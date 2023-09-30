#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <string>

#include <dpp/dpp.h>

#include "readFile.h"

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