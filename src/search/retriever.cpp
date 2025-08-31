#include "retriever.h"
#include <unordered_set>

Retriever::Retriever(Database& db) : db_(db) {}

std::vector<long long> Retriever::retrieve_candidates(const std::vector<std::string>& query_terms) {
    if (query_terms.empty()) {
        return {};
    }

    // Get initial candidates from the first term.
    std::vector<long long> candidate_page_ids = db_.get_pages_for_term(query_terms[0]);
    if (candidate_page_ids.empty()) {
        return {};
    }

    // Filter the candidates using the rest of the terms.
    if (query_terms.size() > 1) {
        for (size_t i = 1; i < query_terms.size(); ++i) {
            const std::string& term = query_terms[i];
            std::vector<long long> pages_for_current_term = db_.get_pages_for_term(term);
            
            // Use a hash set for efficient O(1) average time lookups.
            std::unordered_set<long long> current_term_set(pages_for_current_term.begin(), pages_for_current_term.end());

            std::vector<long long> surviving_candidates;
            surviving_candidates.reserve(candidate_page_ids.size());
            
            for (long long candidate_id : candidate_page_ids) {
                if (current_term_set.count(candidate_id) > 0) {
                    surviving_candidates.push_back(candidate_id);
                }
            }
            
            candidate_page_ids = surviving_candidates;

            if (candidate_page_ids.empty()) {
                break; // No need to check further if we have no candidates left.
            }
        }
    }

    return candidate_page_ids;
}