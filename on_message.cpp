#include <dpp/dpp.h>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "filter.h"
#include "globals.h"
#include "misc.h"

// Forward declaration
dpp::message handle_message(dpp::cluster& bot, const dpp::message_create_t& event);

// Global map to track warning messages and their original messages
dpp::message sent_message;

dpp::message trigger_message(dpp::cluster& bot, const dpp::message& msg) {
    dpp::message response;
	dpp::snowflake m_id = msg.id;
    response.set_content("Potential filter trigger detected. Message: " + msg.content)
        .set_channel_id(msg.channel_id)
        .add_component(
            dpp::component()
            .add_component(
                dpp::component()
                .set_label("Mark as Safe")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_success)
                .set_id("mark_safe__" + std::to_string(msg.id) + "|" + std::to_string(msg.channel_id) + "&" + std::to_string(msg.author.id))
            )
            .add_component(
                dpp::component()
                .set_label("Delete Message")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_danger)
                .set_id("delete_msg_" + std::to_string(msg.id) + "|" + std::to_string(msg.channel_id) + "&" + std::to_string(msg.author.id))
            )
        );
    // std::cout << "Trigger message id" << m_id << std::endl;
    // Store the mapping between warning message and original message
    bot.message_create(response, [&](const dpp::confirmation_callback_t& cb) {
        if (!cb.is_error()) {
            const dpp::message& sent_msg = std::get<dpp::message>(cb.value);
			// std::cout << "Sent message ID: " << sent_msg.id << std::endl;
			sent_message = sent_msg; 
            /*for (auto& msg : moderated_messages) { 
                std::cout << "Moderated message ID: " << msg.first << " -> Original message ID: " << msg.second << std::endl;
            }*/
        }
        else {
            ;
        }
    });
    return sent_message;
}

void setup_on_message(dpp::cluster& bot,
    int &threshold) {

    constexpr const char* PREFIX = "."; // Default prefix

    bot.on_message_create([&](const dpp::message_create_t& event) {
        if (event.msg.author.is_bot()) return;

        const std::string& msg = event.msg.content;
        //std::cout << "Received message: " << msg << std::endl;
        //std::cout << "Raw message: '" << msg << "' size: " << msg.size() << std::endl;
        //std::cout << "Prefix: [" << PREFIX << "] size: " << std::strlen(PREFIX) << std::endl;
        //std::cout << "Prefix size: " << std::strlen(PREFIX) << std::endl;
        if (msg.size() >= std::strlen(PREFIX) && msg.substr(0, std::strlen(PREFIX)) == PREFIX) {
            //std::cout << "Prefix detected";
            std::string command_line = msg.substr(std::strlen(PREFIX));
            std::istringstream iss(command_line);
            std::string command;
            iss >> command;
            std::transform(command.begin(), command.end(), command.begin(), ::tolower);

            if (command == "setgif") {
                //std::cout << "setgif command detected" << std::endl;
                dpp::guild_member member = dpp::find_guild_member(guildID, event.msg.author.id);
                bool is_role = false;
                for (const auto& r : member.get_roles()) {
                    if (r == std::stoll(booster_role)) {
                        is_role = true;
                        std::string gif_url;
                        std::getline(iss, gif_url);
                        if (!gif_url.empty() && gif_url[0] == ' ')
                            gif_url.erase(0, 1);

                        if (gif_url.empty()) {
                            bot.message_create(dpp::message(event.msg.channel_id,
                                "Please provide a gif URL or text, or type none to remove pinggif."));
                            return;
                        }

                        std::string user_id_str = std::to_string(event.msg.author.id);

                        if (gif_url == "none") {
                            manager.remove_gif(user_id_str);
                            updFilterGlobals();
                            bot.message_create(dpp::message(event.msg.channel_id, "Ping gif removed."));
                            return;
                        }

                        // Update in-memory map
                        //std::cout << "About to update pinggif" << std::endl;
                        pinggif[user_id_str] = gif_url;
                        //std::cout << "pinggif updated" << std::endl;

                        // Persist update through ConfigManager
                        if (manager.add_to_gif(user_id_str, gif_url)) {
                            bot.message_create(dpp::message(event.msg.channel_id,
                                "Gif saved and config updated"));
                        }
                        else {
                            bot.message_create(dpp::message(event.msg.channel_id,
                                "Failed to save gif to config"));
                        }
                    }
                }
                if (!is_role) {
                    bot.message_create(dpp::message(event.msg.channel_id, "You're not a booster! Become a booster for this feature."));
                }
            }
        }
        handle_message(bot, event);
        return;
        });
}

dpp::message handle_message(dpp::cluster& bot, const dpp::message_create_t& event) {

    dpp::message response;

    if (event.msg.author.is_bot()) {
        ;
    }
    // Iterate mentions safely
    else if (!event.msg.mentions.empty() && !event.msg.author.is_bot() && bot.me.id != event.msg.author.id) {
        // Create a local copy of mentions to avoid iterator invalidation
        auto mentions = event.msg.mentions;

        for (const auto& [user, member] : mentions) {
            try {
                // Skip invalid users
                if (!user.id || user.is_bot() || user.id == bot.me.id) {
                    continue;
                }

                std::string user_id_str = std::to_string(user.id);
                std::string mention1 = "<@" + user_id_str + ">";
                std::string mention2 = "<@!" + user_id_str + ">";

                // Check if mention actually exists in content
                if (event.msg.content.find(mention1) != std::string::npos ||
                    event.msg.content.find(mention2) != std::string::npos) {

                    auto it = pinggif.find(user_id_str);
                    if (it != pinggif.end()) {
                        bot.message_create(
                            dpp::message(event.msg.channel_id, it->second)
                            .set_reference(event.msg.id)
                        );
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Mention handling error: " << e.what() << '\n';
            }
        }
    }

    // Filter the message content
    if (filter(event.msg.content) > threshold) {
        response = trigger_message(bot, event.msg);
    }
    return response;
}