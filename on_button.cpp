#include <dpp/dpp.h>
#include "config_commands.h"
#include "globals.h"
#include "misc.h"

void setup_on_button(dpp::cluster& bot) {
    bot.on_button_click([&](const dpp::button_click_t& event) {
        size_t pipe_pos = event.custom_id.find('|');
        size_t amp_pos = event.custom_id.find('&');
        int inc = 1;

        if (pipe_pos != std::string::npos || amp_pos != std::string::npos) {
            std::string message_id_str = event.custom_id.substr(11, pipe_pos - 11);
            std::string channel_id_str = event.custom_id.substr(pipe_pos + 1, amp_pos - (pipe_pos + 1));
            std::string user_id_str = event.custom_id.substr(amp_pos + 1);

            dpp::snowflake message_id = std::stoull(message_id_str);
            dpp::snowflake channel_id = std::stoull(channel_id_str);
            dpp::snowflake user_id = std::stoull(user_id_str);
            dpp::user* button_user = dpp::find_user(user_id);
            
            dpp::permission perms = event.command.get_resolved_permission(event.command.usr.id);
            if (!perms.can(dpp::p_manage_guild)) {
                dpp::message msg;
                msg.set_content("You don't have the required permissions to run this!").set_flags(dpp::m_ephemeral);
                event.reply(msg);
                return;
            }

            // Get the message asynchronously
            bot.message_get(message_id, channel_id, [&, message_id, channel_id, user_id_str, event](const dpp::confirmation_callback_t& cc) {
                
                if (cc.is_error()) {
                    std::cout << "Failed to fetch message: " << cc.http_info.body << "\n";
                    event.reply(dpp::ir_update_message, "Could not fetch original message");
                    return;
                }

                dpp::message msg = std::get<dpp::message>(cc.value);
                // std::cout << "Fetched content: " << msg.content << "\n";

                if (event.custom_id.starts_with("delete_msg_")) {
                    bot.message_delete(message_id, channel_id);
                    manager.edit_punishments(user_id_str, inc);
                    event.reply(dpp::ir_update_message, "Message marked as dangerous, deleting...");
                }
                else if (event.custom_id.starts_with("mark_safe_")) {
                    auto detected_words = getWord(msg.content);
                    if (!detected_words.empty()) {
                        for (const auto& [word, amount] : detected_words) {
                            // std::cout << "Whitelisting: " << word << std::endl;
                            manager.add_to_whitelist(word);
                        }
                        updFilterGlobals();
                    }
                    event.reply(dpp::ir_update_message, "Message marked as safe.");
                }
                else {
                    event.reply(dpp::ir_update_message, "Unknown button interaction.");
                }
                });
        }
        });
}
