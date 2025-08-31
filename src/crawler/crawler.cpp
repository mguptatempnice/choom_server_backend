#include "crawler.h"
#include "../network/HTTP_client.h"
#include "../parsing/HTML_parser.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>

std::string get_hostname(const std::string& url) {
    size_t start = url.find("://");
    if (start == std::string::npos) return "";
    start += 3;
    size_t end = url.find('/', start);
    if (end == std::string::npos) return url.substr(start);
    return url.substr(start, end - start);
}

std::string resolve_url(const std::string& base_url, const std::string& relative_url) {
    if (relative_url.empty()) return "";

    if (relative_url.rfind("http://", 0) == 0 || relative_url.rfind("https://", 0) == 0) {
        size_t fragment_pos = relative_url.find('#');
        return relative_url.substr(0, fragment_pos);
    }

    std::string scheme, host, path;
    size_t scheme_end = base_url.find("://");
    if (scheme_end == std::string::npos) return ""; 

    scheme = base_url.substr(0, scheme_end);
    size_t host_end = base_url.find('/', scheme_end + 3);
    if (host_end == std::string::npos) {
        host = base_url.substr(scheme_end + 3);
        path = "/";
    } else {
        host = base_url.substr(scheme_end + 3, host_end - (scheme_end + 3));
        path = base_url.substr(host_end);
    }
    path = path.substr(0, path.find('#'));

    std::string resolved_path;
    if (relative_url[0] == '/') {
        resolved_path = relative_url;
    } else {
        size_t last_slash = path.rfind('/');
        resolved_path = path.substr(0, last_slash + 1) + relative_url;
    }
    std::vector<std::string> path_components;
    std::stringstream path_stream(resolved_path);
    std::string component;
    while (std::getline(path_stream, component, '/')) {
        if (component == "..") {
            if (!path_components.empty()) {
                path_components.pop_back();
            }
        } else if (component != "." && !component.empty()) {
            path_components.push_back(component);
        }
    }

    std::string final_path = "/";
    for (const auto& comp : path_components) {
        final_path += comp + "/";
    }
    if (!resolved_path.empty() && resolved_path.back() != '/' && !final_path.empty()) {
        final_path.pop_back();
    }

    std::string final_url = scheme + "://" + host + final_path;
    size_t fragment_pos = final_url.find('#');
    return final_url.substr(0, fragment_pos);
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current_word;

    for (char ch : text) {
        if (ch >= 'a' && ch <= 'z') {
            current_word += ch;
        } else if (ch >= 'A' && ch <= 'Z') {
            current_word += (ch - 'A' + 'a');
        } else {
            
            if (!current_word.empty()) {
                tokens.push_back(current_word);
                current_word.clear();
            }
        }
    }

    if (!current_word.empty()) {
        tokens.push_back(current_word);
    }

    return tokens;
}

Crawler::Crawler(int num_threads, const std::string& db_path) 
    : num_threads_(num_threads), db_(std::make_unique<Database>(db_path)) {
}

Crawler::~Crawler() {}

void Crawler::start(const std::string& seed_url) {
    
    url_queue_.clear();
    {
        std::lock_guard<std::mutex> lock(visited_mutex_);
        visited_urls_cache_.clear();
    }
    threads_.clear();
    base_hostname_ = get_hostname(seed_url);
    if (base_hostname_.empty()) {
        std::cerr << "Invalid seed URL provided." << std::endl;
        return;
    }
    
    url_queue_.push(seed_url);
    visited_urls_cache_.insert(seed_url);

    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back(&Crawler::worker_thread, this);
    }

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void Crawler::worker_thread() {
    HttpClient client;
    HtmlParser parser;

    while (true) {
        {
            std::lock_guard<std::mutex> lock(visited_mutex_);
            if (visited_urls_cache_.size() > 200) {
                url_queue_.clear();
                break;
            }
        }

        std::string url = url_queue_.pop();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        {
            std::lock_guard<std::mutex> lock(cout_mutex_);
            std::cout << "[" << std::this_thread::get_id() << "] Visiting (" 
                      << get_visited_size() << "/200): " << url << std::endl;
        }

        std::string content = client.fetch(url);
        if (content.empty()) continue; 

        if (parser.parse(content)) {
           
            long long page_id = db_->insert_page(url);

            if (page_id != -1) {
    
                std::string text = parser.getText();
                std::vector<std::string> tokens = tokenize(text);
                std::map<std::string, int> term_frequencies;
                for (const std::string& token : tokens) {
                    term_frequencies[token]++;
                }
    
                if (!term_frequencies.empty()) {
                    std::lock_guard<std::mutex> lock(cout_mutex_);
                    std::string url_short = url.length() > 40 ? url.substr(0, 40) + "..." : url;
                    std::cout << "    [DEBUG] Page " << page_id << " (" << url_short << ") indexed " 
                            << term_frequencies.size() << " UNIQUE tokens." << std::endl;
                }

                db_->insert_terms(page_id, term_frequencies);
            }

            std::vector<std::string> links = parser.getLinks();
            for (const std::string& link : links) {
                if (link.empty() || link[0] == '#' || link.rfind("javascript:", 0) == 0) continue;
                
                std::string absolute_link = resolve_url(url, link);
                
                if (get_hostname(absolute_link) != base_hostname_) continue;

                std::lock_guard<std::mutex> lock(visited_mutex_);
                if (visited_urls_cache_.find(absolute_link) == visited_urls_cache_.end()) {
                    if (visited_urls_cache_.size() <= 200) {
                        visited_urls_cache_.insert(absolute_link);
                        url_queue_.push(absolute_link);
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

int Crawler::get_visited_size() {
    std::lock_guard<std::mutex> lock(visited_mutex_);
    return visited_urls_cache_.size();
}