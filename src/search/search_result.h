#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include <string>

// A simple struct to hold a single search result.
// It will be used to pass data between the Ranker, QueryEngine, and the UI.
struct SearchResult {
    std::string url;
    double score = 0.0;
};

#endif // SEARCH_RESULT_H