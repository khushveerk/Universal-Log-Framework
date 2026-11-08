#ifndef INVERTED_INDEX_HPP
#define INVERTED_INDEX_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class InvertedIndex {
private:
    // Map of token -> Set of event IDs containing that token
    std::unordered_map<std::string, std::unordered_set<std::string>> index_;

    // Helper to tokenize and normalize text (lowercase, remove punctuation)
    std::vector<std::string> tokenize(const std::string& text) const;

public:
    // Index a log event's message or attributes using its event_id
    void addDocument(const std::string& event_id, const std::string& text);

    // Search for documents containing a specific keyword
    std::unordered_set<std::string> searchKeyword(const std::string& keyword) const;

    // Perform an intersection (AND search) across multiple keywords
    std::unordered_set<std::string> searchAND(const std::vector<std::string>& keywords) const;

    // Get total unique indexed terms
    size_t getTermCount() const;
};

#endif // INVERTED_INDEX_HPP