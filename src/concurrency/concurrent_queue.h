#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

class ConcurrentQueue {
public:
    ConcurrentQueue() = default;
    ~ConcurrentQueue() = default;

    void push(const std::string& url);
    std::string pop();
    int size();
    void clear();

private:
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif