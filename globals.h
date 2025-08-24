#include <unordered_map>
#include <unordered_set>
#include <string>
#include "config_manager.h">
#include <dpp/dpp.h>
#include "thread_pool.h"

extern ThreadPool pool;
extern ConfigManager manager;
extern std::unordered_set<std::string> blacklist;
extern std::unordered_set<std::string> whitelist;
extern std::unordered_map<std::string, std::string> substitutes;
extern std::unordered_map<std::string, int> punishments;
extern std::unordered_map<std::string, std::string> pinggif;
extern int threshold;
extern int ratelimit;
extern uint64_t guildID;
extern std::string booster_role;

void updFilterGlobals();