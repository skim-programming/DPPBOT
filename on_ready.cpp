#include <dpp/dpp.h>
#include "on_ready.h"
#include "config_manager.h"
#include "globals.h"

void setup_on_ready(dpp::cluster &bot) {
    dpp::snowflake GUILDID = guildID;
    // Register commands on ready
    bot.on_ready([&bot, GUILDID](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {    
            std::vector<dpp::slashcommand> commands{
                dpp::slashcommand("addww", "Add word to whitelist", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "word", "Word to whitelist", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("addbw", "Add word to blacklist", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "word", "Word to blacklist", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("addsub", "Add word to sub list", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "sub", "Sub in sub list", true))
                .add_option(dpp::command_option(dpp::co_string, "letter", "Letter to get subbed with", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("t", "Set threshold", bot.me.id)
                .add_option(dpp::command_option(dpp::co_integer, "threshold", "Filter threshold", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("removebw", "Remove word from blacklist", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "word", "Word in blacklist", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("removeww", "Remove word from whitelist", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "word", "Word in whitelist", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("removesub", "Remove sub from sub list", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "sub", "Sub in substitute list", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("getpunishments", "Get amount of punishments that a specified user has", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "id", "User's id/@", true)),
                dpp::slashcommand("setratelimit", "Set how often commands can be run", bot.me.id)
                .add_option(dpp::command_option(dpp::co_integer, "limit", "Rate", true))
                .set_default_permissions(dpp::p_manage_guild),
                dpp::slashcommand("getchunk", "Get detected chunk in a word", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "word", "Detected word", true)),
				dpp::slashcommand("viewwl", "View whitelist", bot.me.id)
				.set_default_permissions(dpp::p_manage_guild),
				dpp::slashcommand("viewbl", "View blacklist", bot.me.id)
				.set_default_permissions(dpp::p_manage_guild),
				dpp::slashcommand("viewsubs", "View subs", bot.me.id)
				.set_default_permissions(dpp::p_manage_guild),
				dpp::slashcommand("getpinggif", "Get ping gif of user", bot.me.id)
                .add_option(dpp::command_option(dpp::co_user, "id", "User id to get ping gif of", true))
				.set_default_permissions(dpp::p_use_application_commands),
                dpp::slashcommand("getword", "Get detected word", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "sentence", "Detected sentence", true))
                .set_default_permissions(dpp::p_manage_guild),
				dpp::slashcommand("getfinished", "Get all transformations of text", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "text", "Text to transform", true))
				.set_default_permissions(dpp::p_manage_guild),
				dpp::slashcommand("setboosterrole", "Set the booster role", bot.me.id)
                .add_option(dpp::command_option(dpp::co_role, "role_id", "Role to set as booster role", true))
				.set_default_permissions(dpp::p_manage_guild),
				dpp::slashcommand("setgif", "Set ping gif for user", bot.me.id)
                .add_option(dpp::command_option(dpp::co_user, "user_id", "User to set ping gif for", true))
                .add_option(dpp::command_option(dpp::co_string, "gif_url", "Gif url to set as ping gif", true))
				.set_default_permissions(dpp::p_use_application_commands)
            };
            bot.guild_bulk_command_create(commands, GUILDID, [](const dpp::confirmation_callback_t& cb) {
                if (cb.is_error()) {
                    std::cerr << "Command creation failed: " << cb.get_error().message << std::endl;
                }
                else {
                    std::cout << "Commands created successfully." << std::endl;
                }
                });
        }
        std::cout << "Logged in as " << bot.me.username << std::endl;
    });
}