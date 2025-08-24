#pragma once
#include <string>
#include <unordered_set>

// Declare filter-related functions
int filter(const std::string& message);
void testFilter();
std::string normalize(const std::string& input);
std::string remove_whitelist_words(std::string text, const std::unordered_set<std::string>& whitelist_patterns);
std::string findSub(const std::string& ch);
std::string remove_duplicates(const std::string& s);
std::vector<std::pair<std::string, int>> get_highest_chunks(const std::string& input, const std::unordered_set<std::string>& patterns);