#include <dpp/dpp.h>
#include <unordered_set>
#include "config_manager.h"
#include "filter.h"
#include "config_commands.h"
#include "globals.h"
#include "misc.h"

// Declare last_command_time and ratelimit as static to persist across function calls
static std::unordered_map<dpp::snowflake, std::chrono::steady_clock::time_point> last_command_time;
static int ratelimit = 1; // Default rate limit in seconds

void commands(dpp::cluster& bot) {

    bot.on_slashcommand([&](const dpp::slashcommand_t& event) {
        const std::string cmd = event.command.get_command_name();
        const auto user_id = event.command.member.user_id;
        const auto now = std::chrono::steady_clock::now();

        // Check if the user has a recorded last command time
        if (last_command_time.count(user_id) && cmd != "setratelimit") {
            // Calculate the time difference
            auto time_since_last = now - last_command_time[user_id];
            if (time_since_last < std::chrono::seconds(ratelimit)) {
                event.reply("Please wait before using another command.");
                return;
            }
        }

        // Update the last command time for the user
        last_command_time[user_id] = now;

        if (cmd == "addww") {
            try {
                std::string word = std::get<std::string>(event.get_parameter("word"));
                manager.add_to_whitelist(word);
                updFilterGlobals();
                event.reply("Word added to whitelist: " + word);
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid 'word' parameter.");
            }
        }

        else if (cmd == "addbw") {
            try {
                std::string word = std::get<std::string>(event.get_parameter("word"));
                manager.add_to_blacklist(word);
                updFilterGlobals();
                event.reply("Word added to blacklist: " + word);
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid 'word' parameter.");
            }
        }

        else if (cmd == "addsub") {
            try {
                std::string sub = std::get<std::string>(event.get_parameter("sub"));
                std::string letter = std::get<std::string>(event.get_parameter("letter"));
                manager.add_to_sub(sub, letter);
                updFilterGlobals();
                event.reply("Substitution added.");
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid sub/letter.");
            }
        }

        else if (cmd == "t") {
            try {
                int threshold = static_cast<int>(std::get<int64_t>(event.get_parameter("threshold")));
                manager.set_threshold(threshold);
                updFilterGlobals();
                event.reply("Threshold received: " + std::to_string(threshold));
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid threshold.");
            }
        }
        else if (cmd == "removebw") {
            try {
                std::string word = std::get<std::string>(event.get_parameter("word"));
                manager.remove_blacklist(word);
                updFilterGlobals();
                event.reply("Word removed.");
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid word.");
            }
        }
        else if (cmd == "removeww") {
            try {
                std::string word = std::get<std::string>(event.get_parameter("word"));
                manager.remove_whitelist(word);
                updFilterGlobals();
                event.reply("Word removed.");
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid word.");
            }
        }
        else if (cmd == "removesub") {
            try {
                std::string sub = std::get<std::string>(event.get_parameter("sub"));
                manager.remove_sub(sub);
                updFilterGlobals();
                event.reply("Sub removed.");
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid sub.");
            }
        }
        else if (cmd == "getpunishments") {
            try {
                std::string id = std::get<std::string>(event.get_parameter("id"));
                // Clean the ID string (remove <@!> etc.)
                id.erase(std::remove_if(id.begin(), id.end(),
                    [](char c) { return !std::isdigit(c); }),
                    id.end());

                if (id.empty()) {
                    event.reply("Invalid user ID format");
                    return;
                }

                else if (manager.config.punishments[id] <= 0) {
                    std::cout << "Less than or equal to 0" << std::endl;
                    int inc = 0;
                    manager.edit_punishments(id, inc);
                }

                // Check if punishments map exists and has the user
                if (manager.config.punishments.count(id)) {
                    event.reply("User has " + std::to_string(manager.config.punishments[id]) + " punishments.");
                }
                else {
                    event.reply("User has no punishments.");
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Punishment check error: " << e.what() << '\n';
                event.reply("An error occurred while checking punishments");
            }
        }
        else if (cmd == "setratelimit") {
            try {
                int new_limit = static_cast<int>(std::get<int64_t>(event.get_parameter("limit")));
                if (new_limit < 0) {
                    event.reply("Rate limit must be a positive integer.");
                    return;
                }
                ratelimit = new_limit; // Update the static rate limit
                manager.set_rate(new_limit);
                updFilterGlobals();
                event.reply("Rate limit set to " + std::to_string(new_limit));
            }
            catch (const std::bad_variant_access&) {
                event.reply("Missing or invalid limit parameter.");
            }
        }
        // Example usage in your slash command handler
        else if (cmd == "getchunk") {
            try {
                std::string word = std::get<std::string>(event.get_parameter("word"));
                std::string cleaned = normalize(word);
                std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), ::tolower);
                cleaned = remove_whitelist_words(cleaned, whitelist);

                std::string substituted;
                for (char c : cleaned) {
                    std::string ch(1, c);
                    substituted += findSub(ch);
                }
                std::string no_spaces = substituted;
                no_spaces.erase(std::remove(no_spaces.begin(), no_spaces.end(), ' '), no_spaces.end());
                std::string deduped = remove_duplicates(no_spaces);

                std::vector<std::string> transforms = { cleaned, substituted, no_spaces, deduped };
                std::vector<std::pair<std::string, int>> best_chunks;
                int best_score = 0;

                for (const auto& text : transforms) {
                    auto chunks = get_highest_chunks(text, blacklist);
                    if (!chunks.empty() && chunks[0].second > best_score) {
                        best_score = chunks[0].second;
                        best_chunks = chunks;
                    }
                }

                if (!best_chunks.empty()) {
                    std::string reply = "Highest detected chunk(s):\n";
                    for (const auto& [chunk, score] : best_chunks) {
                        reply += "`" + chunk + "` (score: " + std::to_string(score) + ")\n";
                    }
                    event.reply(reply);
                }
                else {
                    event.reply("No suspicious chunks detected.");
                }
            }
            catch (...) {
                event.reply("Missing or invalid 'word' parameter.");
            }
        }
        else if (cmd == "getword") {
            try {
                std::string word = std::get<std::string>(event.get_parameter("sentence")); // I don't know why i named the variable word when its the sentence, sorr
                std::vector<std::pair<std::string, int>> best_words = getWord(word);

                if (!best_words.empty()) {
                    std::string reply = "Highest detected word(s):\n";
                    for (const auto& [word, score] : best_words) {
                        reply += "`" + word + "` (score: " + std::to_string(score) + ")\n";
                    }
                    event.reply(reply);
                }
            }
            catch (const std::exception& e) {
                event.reply(std::string("Error: ") + e.what());
            }
            }
        else if (cmd == "getfinished") {
			std::string text = std::get<std::string>(event.get_parameter("text"));
            std::string cleaned = normalize(text);
            std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), ::tolower);
            cleaned = remove_whitelist_words(cleaned, whitelist);

            std::string substituted;
            for (char c : cleaned) substituted += findSub(std::string(1, c));

            std::string no_spaces = substituted;
            no_spaces.erase(std::remove(no_spaces.begin(), no_spaces.end(), ' '), no_spaces.end());
            std::string deduped = remove_duplicates(no_spaces);
			event.reply("Normalized/Whitelisted: " + cleaned + "\nSubstituted: " + substituted + "\nNo spaces: " + no_spaces + "\nDeduped: " + deduped);
        }

        else if (cmd == "viewwl") {
            std::string whitelist_str = "Whitelist:\n";
            for (const auto& word : whitelist) {
                whitelist_str += "- " + word + "\n";
            }
            event.reply(whitelist_str);
        }
        else if(cmd == "viewbl"){
            std::string blacklist_str = "Blacklist:\n";
            for (const auto& word : blacklist) {
                blacklist_str += "- " + word + "\n";
            }
            event.reply(blacklist_str);
        }
        else if (cmd == "viewsubs") {
            std::string subs_str = "Substitutions:\n";
            for (const auto& [sub, letter] : substitutes) {
                subs_str += "- " + sub + " -> " + letter + "\n";
            }
            event.reply(subs_str);
        }
        else if(cmd=="getpinggif"){
			std::string id = std::to_string(std::get<dpp::snowflake>(event.get_parameter("id")));
            if (pinggif.find(id) != pinggif.end()) {
                event.reply("Ping GIF/message: " + pinggif[id]);
            } else {
                event.reply("No GIF found for ID: " + id);
			}
		}
        else if (cmd == "setboosterrole") {
			dpp::snowflake role_id = std::get<dpp::snowflake>(event.get_parameter("role_id"));
            manager.set_booster_role(std::to_string(role_id));
            updFilterGlobals();
			event.reply("Booster role updated.");
        }
        else if (cmd == "setgif") {
			dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user_id"));
            std::string gif_url = std::get<std::string>(event.get_parameter("gif_url"));
            if (gif_url.empty()) {
                event.reply("Please provide a gif URL or text, or type none to remove pinggif.");
                return;
            }
            std::string user_id_str = std::to_string(user_id);
            if (gif_url == "none") {
                manager.remove_gif(user_id_str);
                updFilterGlobals();
                event.reply("Ping gif removed.");
                return;
            }
            // Update in-memory map
            pinggif[user_id_str] = gif_url;
            // Persist update through ConfigManager
            if (manager.add_to_gif(user_id_str, gif_url)) {
                event.reply("Gif saved and config updated");
            }
            else {
                event.reply("Failed to save gif to config");
            }

        }
        else {
            event.reply("Unknown command: " + cmd + " or missing parameters.");
        }
    });
}
