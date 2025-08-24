#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include "config_manager.h"
#include "filter.h"
#include "globals.h"

// Split for cpp, similar to py
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::pair<std::string, int>> getWord(const std::string& sentence) {
    std::string cleaned = normalize(sentence);
    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), ::tolower);
    cleaned = remove_whitelist_words(cleaned, whitelist);

    std::string substituted;
    for (char c : cleaned) substituted += findSub(std::string(1, c));

    std::vector<std::string> words = split(substituted, ' ');
    std::vector<std::future<std::pair<std::string, int>>> futures;

    // Enqueue each word for scoring with filter()
    for (const auto& word : words) {
        futures.push_back(pool.enqueue(std::function<std::pair<std::string, int>()>(
            [word]() -> std::pair<std::string, int> {
                int score = filter(word);
                return { word, score };
            }
        )));
    }

    std::vector<std::pair<std::string, int>> best_words;
    int best_score = 0;

    // Collect results
    for (auto& f : futures) {
        auto [word, score] = f.get();
        if (score > best_score) {
            best_score = score;
            best_words.clear();
            best_words.emplace_back(word, score);
        }
        else if (score == best_score && !word.empty()) {
            best_words.emplace_back(word, score);
        }
    }

    return best_words;
}
