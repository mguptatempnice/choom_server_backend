#include "concurrent_queue.h"

void ConcurrentQueue::push(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);

    queue_.push(url);
    cond_.notify_one();
}

std::string ConcurrentQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);

    cond_.wait(lock, [this] { return !queue_.empty(); });

    std::string url = queue_.front();
    queue_.pop();

    return url;
}

int ConcurrentQueue::size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void ConcurrentQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<std::string> empty;
    std::swap(queue_, empty);
}