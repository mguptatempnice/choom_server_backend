#ifndef RETRIEVER_H
#define RETRIEVER_H

#include "../database/database.h"
#include <string>
#include <vector>

class Retriever {
public:
    // Constructor takes a reference to the database.
    Retriever(Database& db);

    // The single public method: takes tokenized query terms and finds matching pages.
    std::vector<long long> retrieve_candidates(const std::vector<std::string>& query_terms);

private:
    Database& db_; // A reference, not a copy.
};

#endif // RETRIEVER_H```