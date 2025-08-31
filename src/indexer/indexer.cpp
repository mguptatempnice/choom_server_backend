#include "indexer.h"
#include <iostream>

Indexer::Indexer() : is_crawling_(false) {
    unsigned int num_threads = 4;
    
    crawler_ = std::make_unique<Crawler>(num_threads, "search_engine.db");
}

bool Indexer::start_crawl(const std::string& seed_url) {
    //ensuring that only one page gets indexed at a time
    bool already_crawling = is_crawling_.exchange(true);
    if (already_crawling) {
        std::cout << "[INDEXER] A crawl is already in progress. Ignoring new request." << std::endl;
        return false;
    }

    std::thread crawl_thread([this, seed_url] {
        std::cout << "[INDEXER] Starting new crawl with seed: " << seed_url << std::endl;
        
        crawler_->start(seed_url);
        
        std::cout << "[INDEXER] Crawl finished for seed: " << seed_url << std::endl;
        is_crawling_.store(false);
    });
    crawl_thread.detach();

    return true;
}