#include <regex>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>
#include <unicode/errorcode.h>
#include <future>
#include "config_manager.h"
#include "globals.h"

// Normalize a UTF-8 std::string using NFKC
std::string normalize(const std::string& input) {
    UErrorCode errorCode = U_ZERO_ERROR;
    const icu::Normalizer2* normalizer = icu::Normalizer2::getInstance(nullptr, "nfkc", UNORM2_COMPOSE, errorCode);
    std::string output;
    if (U_FAILURE(errorCode)) {
        throw std::runtime_error("Failed to get ICU Normalizer2 instance");
    }
    if (input.empty()) {
        // Handle empty string case or return early
        return input;
    }
    else {
        icu::UnicodeString unicodeInput = icu::UnicodeString::fromUTF8(input);
        icu::UnicodeString normalized;
        normalizer->normalize(unicodeInput, normalized, errorCode);
        if (U_FAILURE(errorCode)) {
            throw std::runtime_error("ICU normalization failed");
        }
        normalized.toUTF8String(output);
        return output;
    }
}

int levenshtein_distance(const std::string& s1, const std::string& s2) {
    size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));
    for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({ dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost });
        }
    }
    return dp[len1][len2];
}

int ratio(const std::string& s1, const std::string& s2) {
    int dist = levenshtein_distance(s1, s2);
    int max_len = std::max(s1.size(), s2.size());
    return max_len == 0 ? 100 : static_cast<int>((1.0 - (double)dist / max_len) * 100);
}

std::string findSub(const std::string& ch) {
    auto it = substitutes.find(ch);
    return it != substitutes.end() ? it->second : ch;
}

std::string remove_duplicates(const std::string& s) {
    std::unordered_set<char> seen;
    std::string result;
    for (char c : s) {
        if (seen.insert(c).second) {
            result += c;
        }
    }
    return result;
}

std::string remove_whitelist_words(std::string text, const std::unordered_set<std::string>& whitelist_patterns) {
    for (const auto& pattern : whitelist_patterns) {
        if (pattern.empty()) continue;
        std::string lowered = pattern;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);

        // Treat underscores as wildcards
        std::string regex_pattern;
        for (char c : lowered) {
            regex_pattern += (c == '_') ? '.' : c;
        }

        try {
            std::regex re(regex_pattern, std::regex_constants::icase);
            text = std::regex_replace(text, re, "");
        }
        catch (const std::regex_error&) {
            continue;
        }
    }
    return text;
}

std::tuple<int, std::string> blregex(const std::unordered_set<std::string>& patterns, const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);

    for (const auto& pattern : patterns) {
        if (pattern.find('_') == std::string::npos) continue;
        std::string regex_pattern;
        for (char c : pattern) regex_pattern += (c == '_') ? '.' : c; 

        try {
            std::regex re(regex_pattern, std::regex_constants::icase);
            if (std::regex_search(lower_text, re)) {
                return { 100, pattern };
            }
        }
        catch (const std::regex_error&) {
            continue;
        }
    }
    return { 0, "" };
}

std::vector<std::pair<std::string, int>> get_highest_chunks(const std::string& input, const std::unordered_set<std::string>& patterns) {
    int highest = 0;
    std::vector<std::pair<std::string, int>> result;

    for (const auto& pattern : patterns) {
        if (pattern.find('_') != std::string::npos) continue;
        std::string lower_pattern = pattern;
        std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);
        size_t plen = lower_pattern.size();

        if (input.size() < plen) continue;

        for (size_t i = 0; i <= input.size() - plen; i++) {
            std::string chunk = input.substr(i, plen);
            int score = ratio(chunk, lower_pattern);
            if (score > highest) {
                highest = score;
                result.clear();
                result.emplace_back(chunk, score);
            }
            else if (score == highest) {
                result.emplace_back(chunk, score);
            }
        }
    }
    return result;
}

int filter(const std::string& message) {
    std::string cleaned = normalize(message);
    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), ::tolower);
    cleaned = remove_whitelist_words(cleaned, whitelist);

    std::string substituted;
    for (char c : cleaned) substituted += findSub(std::string(1, c));

    std::string no_spaces = substituted;
    no_spaces.erase(std::remove(no_spaces.begin(), no_spaces.end(), ' '), no_spaces.end());
    std::string deduped = remove_duplicates(no_spaces);

    std::vector<std::string> transforms = { cleaned, substituted, no_spaces, deduped };
    std::vector<std::future<int>> futures;

    for (const auto& text : transforms) {
        futures.push_back(pool.enqueue([text]() -> int {
            int local_high = 0; 
            auto [score, _] = blregex(blacklist, text);
            local_high = std::max(local_high, score);

            auto fuzzy = get_highest_chunks(text, blacklist);
            if (!fuzzy.empty()) local_high = std::max(local_high, fuzzy[0].second);

            return local_high;
            }));
    }

    int highest = 0;
    for (auto& f : futures) highest = std::max(highest, f.get());
    return highest;
}





void testFilter() {
    std::vector<std::string> tests = {
        "nigga", "nigger", "n1gger", "n1gg3r5", "niiiiiigggeeerrr",
        "hello knitter", "ter", "niger", "knitwear", "knitted",
        "nigerian", "bigger", "nbigger", "anger", "clean message"
    };

    for (const auto& msg : tests) {
        int score = filter(msg);
        std::cout << std::left << std::setw(30) << msg << ": " << score << std::endl;
    }
}
