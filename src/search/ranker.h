#ifndef RANKER_H
#define RANKER_H

#include "../database/database.h"
#include "search_result.h"
#include <vector>

class Ranker {
public:
    Ranker(Database& db);

    // Takes candidate pages and will eventually rank them.
    std::vector<SearchResult> rank_results(const std::vector<long long>& candidate_ids);

private:
    Database& db_;
};

#endif // RANKER_H