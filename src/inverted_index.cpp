#include "inverted_index.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> InvertedIndex::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        // Remove punctuation and convert to lowercase for case-insensitive indexing
        std::string cleaned = "";
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                cleaned += std::tolower(static_cast<unsigned char>(c));
            }
        }
        if (!cleaned.empty()) {
            tokens.push_back(cleaned);
        }
    }
    return tokens;
}

void InvertedIndex::addDocument(const std::string& event_id, const std::string& text) {
    std::vector<std::string> tokens = tokenize(text);
    for (const auto& token : tokens) {
        index_[token].insert(event_id);
    }
}

std::unordered_set<std::string> InvertedIndex::searchKeyword(const std::string& keyword) const {
    // Normalize query keyword the same way
    std::string query_token = "";
    for (char c : keyword) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            query_token += std::tolower(static_cast<unsigned char>(c));
        }
    }

    auto it = index_.find(query_token);
    if (it != index_.end()) {
        return it->second;
    }
    return {}; // Return empty set if not found
}

std::unordered_set<std::string> InvertedIndex::searchAND(const std::vector<std::string>& keywords) const {
    if (keywords.empty()) return {};

    std::unordered_set<std::string> result_set = searchKeyword(keywords[0]);

    for (size_t i = 1; i < keywords.size(); ++i) {
        std::unordered_set<std::string> next_set = searchKeyword(keywords[i]);
        std::unordered_set<std::string> intersection;

        for (const auto& id : result_set) {
            if (next_set.count(id)) {
                intersection.insert(id);
            }
        }
        result_set = intersection;
        if (result_set.empty()) break; // Early exit if no matches
    }

    return result_set;
}

size_t InvertedIndex::getTermCount() const {
    return index_.size();
}