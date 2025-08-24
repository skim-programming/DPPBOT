#include "config_manager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>

using json = nlohmann::json;

const std::string path = "data.json"; // Json path

void to_json(json& j, const BotConfig& cfg) {
    j = json{
        {"prefix", cfg.prefix},
        {"pinggif", cfg.pinggif},
        {"blacklist", cfg.blacklist},
        {"whitelist", cfg.whitelist},
        {"substitutes", cfg.substitutes},
        {"punishments", cfg.punishments},
        {"guildID", cfg.guildID},
        {"threshold", cfg.threshold},
        {"ratelimit", cfg.ratelimit},
		{"booster_role", cfg.booster_role}
    };
}

void from_json(const json& j, BotConfig& cfg) {
    cfg.prefix = j.value("prefix", ".");
    cfg.pinggif = j.value("pinggif", std::unordered_map<std::string, std::string>{});
    cfg.blacklist = j.value("blacklist", std::unordered_set<std::string>{});
    cfg.whitelist = j.value("whitelist", std::unordered_set<std::string>{});
    cfg.substitutes = j.value("substitutes", std::unordered_map<std::string, std::string>{});
    cfg.punishments = j.value("punishments", std::unordered_map<std::string, int>{});
    cfg.guildID = j.value("guildID", 0ULL);
    cfg.threshold = j.value("threshold", 75);
	cfg.ratelimit = j.value("ratelimit", 1); 
	cfg.booster_role = j.value("booster_role", "");
}

bool ConfigManager::load_config() {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            config = BotConfig(); // Initialize fresh config
            return save_config();
        }

        json j;
        file >> j;
        config = j.get<BotConfig>();
        return true;
    }
    catch (std::exception e) {
        config = BotConfig(); // Reset to defaults on error
        std::cout << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::save_config() {
    try {
        std::ofstream file(path);
        if (!file.is_open()) return false;

        json j = config;
        file << j.dump(4);
        return true;
    }
    catch (...) {
        return false;
    }
}
bool ConfigManager::save_config_async() {
    auto cfg_copy = config; // make a copy
    std::async(std::launch::async, [cfg_copy] {
        std::ofstream file(path);
        if (!file.is_open()) return;
        nlohmann::json j = cfg_copy;
        file << j.dump(4);
        });
    return true;
}

const BotConfig& ConfigManager::get_config() const {
    return config;
}

bool ConfigManager::add_to_blacklist(const std::string& word) {
    std::lock_guard<std::mutex> lock(config_mutex);
    config.blacklist.insert(word);
    save_config_async();
    return true;
}

bool ConfigManager::add_to_whitelist(const std::string& word) {
    std::lock_guard<std::mutex> lock(config_mutex);
    config.whitelist.insert(word);
    save_config();
    return true;
}

bool ConfigManager::add_to_sub(const std::string& sub, const std::string& letter) {
    std::lock_guard<std::mutex> lock(config_mutex);
    config.substitutes[sub] = letter;
    save_config();
    return true;
}

bool ConfigManager::add_to_gif(const std::string& id, const std::string& url) {
    std::lock_guard<std::mutex> lock(config_mutex);
    config.pinggif[id] = url;
    save_config();
    return true;
}

bool ConfigManager::set_threshold(const int& threshold) {
    std::lock_guard<std::mutex> lock(config_mutex);
    config.threshold = threshold;
    save_config();
    return true;
}

bool ConfigManager::remove_blacklist(const std::string& word) {
    std::lock_guard<std::mutex> lock(config_mutex);
    auto it = std::find(config.blacklist.begin(), config.blacklist.end(), word);
    if (it != config.blacklist.end()) {
        config.blacklist.erase(it);
        save_config();
        return true;
    }
    return false;
}

bool ConfigManager::remove_whitelist(const std::string& word) {
    std::lock_guard<std::mutex> lock(config_mutex);
    auto it = std::find(config.whitelist.begin(), config.whitelist.end(), word);
    if (it != config.whitelist.end()) {
        config.whitelist.erase(it);
        save_config();
        return true;
    }
    return false;
}

bool ConfigManager::remove_sub(const std::string& sub) {
    auto it = config.substitutes.find(sub);
    if (it != config.substitutes.end()) {
        config.substitutes.erase(it);
        save_config();
        return true;
    }
    return false;
}

bool ConfigManager::remove_gif(const std::string id) {
    std::lock_guard<std::mutex> lock(config_mutex);
    auto it = config.pinggif.find(id);
    if (it != config.pinggif.end()) {
        config.pinggif.erase(it);
        save_config();
        return true;
    }
    return false;
}

bool ConfigManager::edit_punishments(const std::string& id, int& amount) {
    std::lock_guard<std::mutex> lock(config_mutex);
    config.punishments[id] += amount;
    if (config.punishments[id] <= 0) {
        config.punishments.erase(id);
    }
    save_config();
    return true;
}   

bool ConfigManager::set_rate(const int& rate) {
    try {
        std::lock_guard<std::mutex> lock(config_mutex);
        config.ratelimit = rate;
        save_config();
        return true;
    }
	catch (...) {
		return false;
	}
}

bool ConfigManager::set_booster_role(const std::string& role_id) {
    try {
        std::lock_guard<std::mutex> lock(config_mutex);
        config.booster_role = role_id;
        save_config();
        return true;
    }
    catch (...) {
        return false;
    }
}