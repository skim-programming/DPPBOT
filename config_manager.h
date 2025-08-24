#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <dpp/dpp.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

// Global variables
extern const std::string path;

struct BotConfig {
    std::string prefix = ".";
    std::unordered_map<std::string, std::string> pinggif;
    std::unordered_set<std::string> blacklist;
    std::unordered_set<std::string> whitelist;
    std::unordered_map<std::string, std::string> substitutes;
    std::unordered_map<std::string, int> punishments;
    std::string booster_role = "";
    int ratelimit;
    uint64_t guildID;
    int threshold;
};

// Custom JSON serialization
void to_json(nlohmann::json& j, const BotConfig& cfg);
void from_json(const nlohmann::json& j, BotConfig& cfg);

class ConfigManager {
public:
    std::mutex config_mutex;
    BotConfig config;

    bool load_config();
    bool save_config();
	bool save_config_async();
    const BotConfig& get_config() const;

    bool add_to_blacklist(const std::string& word);
    bool add_to_whitelist(const std::string& word);
    bool add_to_sub(const std::string& sub, const std::string& letter);
    bool add_to_gif(const std::string& id, const std::string& url);
    bool set_threshold(const int& threshold);
    bool remove_blacklist(const std::string& word);
    bool remove_whitelist(const std::string& word);
    bool remove_sub(const std::string& sub);
    bool remove_gif(const std::string id);
    bool edit_punishments(const std::string& id, int& amount);
    bool set_rate(const int& rate);
    bool set_booster_role(const std::string& role_id);
};