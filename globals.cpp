#include "globals.h"
#include "config_manager.h"
#include "thread_pool.h"

ConfigManager manager;
std::unordered_set<std::string> blacklist;
std::unordered_set<std::string> whitelist;
std::unordered_map<std::string, std::string> substitutes;
std::unordered_map<std::string, int> punishments;
std::unordered_map<std::string, std::string> pinggif;
int threshold = 75; // Defaults
int ratelimit = 3;
uint64_t guildID = 0;
ThreadPool pool(std::thread::hardware_concurrency());
std::string booster_role;

void updFilterGlobals() { // Purely memory updates, real updates are done with save_config()
    if (manager.load_config()) {
		const BotConfig& cfg = manager.get_config();
        substitutes = cfg.substitutes;
        blacklist = cfg.blacklist;
        whitelist = cfg.whitelist;
        threshold = cfg.threshold;
        punishments = cfg.punishments;
		pinggif = cfg.pinggif;
        ratelimit = cfg.ratelimit;
        guildID = cfg.guildID;
		booster_role = cfg.booster_role;
		// std::cout << "Guild ID: " << guildID << std::endl;
		// std::cout << "Config loaded successfully." << std::endl;
    }
    else {
		std::cout << "Failed to load config, using defaults." << std::endl;
    }
}