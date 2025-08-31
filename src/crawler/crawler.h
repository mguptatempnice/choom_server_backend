#ifndef CRAWLER_H
#define CRAWLER_H

#include "../concurrency/concurrent_queue.h"
#include "../database/database.h" 
#include <string>
#include <vector>
#include <thread>
#include <unordered_set>
#include <mutex>
#include <memory>

class Crawler {
public:
    Crawler(int num_threads, const std::string& db_path);
    ~Crawler();

    void start(const std::string& seed_url);
    int get_visited_size();

private:
    void worker_thread();
    
    int num_threads_;
    std::string base_hostname_;

    // db pointer 
    std::unique_ptr<Database> db_;

    std::vector<std::thread> threads_;
    ConcurrentQueue url_queue_;

    // urls for storing
    std::unordered_set<std::string> visited_urls_cache_;
    std::mutex visited_mutex_;
    std::mutex cout_mutex_;
};

#endif 