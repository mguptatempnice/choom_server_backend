#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include "retriever.h"
#include "ranker.h"
#include "search_result.h"
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

class QueryEngine {
public:
    // Constructor creates the components it needs.
    QueryEngine(Database& db);

    // The single public entry point for the UI to call.
    std::vector<SearchResult> search(const std::string& query_string);

private:
    // Private helper for tokenizing.
    std::vector<std::string> tokenize(const std::string& text);
    
    Database& db_;
    
    // The QueryEngine owns its components.
    std::unique_ptr<Retriever> retriever_;
    std::unique_ptr<Ranker> ranker_;
};

#endif // QUERY_ENGINE_H