#ifndef INDEXER_H
#define INDEXER_H

#include "../crawler/crawler.h"
#include <string>
#include <memory>
#include <atomic>
#include <thread>

class Indexer {
public:
    Indexer();
    bool start_crawl(const std::string& seed_url);

private:
    std::unique_ptr<Crawler> crawler_;
   
    std::atomic<bool> is_crawling_;
};

#endif