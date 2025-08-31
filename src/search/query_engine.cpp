#include "query_engine.h"
#include <sstream>
#include <algorithm>
#include <cctype>

// We keep the tokenizer here as it's part of the query processing logic.
std::vector<std::string> QueryEngine::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(),
                                  [](char c) { return std::ispunct(c); }),
                   word.end());
        std::transform(word.begin(), word.end(), word.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        if (!word.empty()) {
            tokens.push_back(word);
        }
    }
    return tokens;
}


QueryEngine::QueryEngine(Database& db) : db_(db) {
    // The orchestrator creates its workers.
    retriever_ = std::make_unique<Retriever>(db_);
    ranker_ = std::make_unique<Ranker>(db_);
}

std::vector<SearchResult> QueryEngine::search(const std::string& query_string) {
    // 1. Tokenize the query.
    std::vector<std::string> query_terms = tokenize(query_string);
    if (query_terms.empty()) {
        return {};
    }

    // 2. Pass the terms to the Retriever component.
    std::vector<long long> candidate_ids = retriever_->retrieve_candidates(query_terms);

    // 3. Pass the candidate IDs to the Ranker component.
    return ranker_->rank_results(candidate_ids);
}