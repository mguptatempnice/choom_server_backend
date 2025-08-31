#include "ranker.h"

Ranker::Ranker(Database& db) : db_(db) {}

std::vector<SearchResult> Ranker::rank_results(const std::vector<long long>& candidate_ids) {
    std::vector<SearchResult> results;
    results.reserve(candidate_ids.size());

    // For now, we just convert IDs to URLs and assign a dummy score.
    // Later, this is where the TF-IDF logic will go.
    for (long long id : candidate_ids) {
        SearchResult res;
        res.url = db_.get_url_for_page_id(id);
        res.score = 1.0; // Dummy score
        results.push_back(res);
    }
    
    return results;
}