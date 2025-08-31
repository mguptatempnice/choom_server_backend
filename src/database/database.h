#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include <mutex>
#include <map>

class Database {
public:
    Database(const std::string& db_path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    long long insert_page(const std::string& url);

    void insert_terms(long long page_id, const std::map<std::string, int>& term_frequencies);

    std::vector<long long> get_pages_for_term(const std::string& term);
    std::string get_url_for_page_id(long long page_id);

private:
    void create_tables();

    sqlite3* db_handle_;
    std::mutex db_mutex_;
};

#endif 