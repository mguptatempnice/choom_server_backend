#include "database.h"
#include <iostream>

Database::Database(const std::string& db_path) : db_handle_(nullptr) {
    int rc = sqlite3_open_v2(db_path.c_str(), &db_handle_,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                             nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "FATAL: Cannot open database: " << sqlite3_errmsg(db_handle_) << std::endl;
        db_handle_ = nullptr;
        return;
    }
    std::cout << "Database opened successfully: " << db_path << std::endl;
    create_tables();
}

Database::~Database() {
    if (db_handle_) {
        sqlite3_close(db_handle_);
    }
}

void Database::create_tables() {
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char* sql =
        "CREATE TABLE IF NOT EXISTS pages ("
        "    id INTEGER PRIMARY KEY,"
        "    url TEXT NOT NULL UNIQUE"
        ");"
        "CREATE TABLE IF NOT EXISTS inverted_index ("
        "    term TEXT NOT NULL,"
        "    page_id INTEGER NOT NULL,"
        "    FOREIGN KEY (page_id) REFERENCES pages (id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_term ON inverted_index (term);";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_handle_, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in create_tables: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        std::cout << "Tables verified/created successfully." << std::endl;
    }
}

long long Database::insert_page(const std::string& url) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char* sql = "INSERT OR IGNORE INTO pages (url) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing statement (insert_page): " << sqlite3_errmsg(db_handle_) << std::endl;
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);

    long long page_id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        page_id = sqlite3_last_insert_rowid(db_handle_);
    }
    sqlite3_finalize(stmt);
    if (url.length() > 0 && page_id == 0) {
        const char* select_sql = "SELECT id FROM pages WHERE url = ?;";
        if (sqlite3_prepare_v2(db_handle_, select_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                page_id = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }
    return page_id;
}

void Database::insert_terms(long long page_id, const std::map<std::string, int>& term_frequencies) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char* sql = "INSERT INTO inverted_index (term, page_id) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing statement (insert_terms): " << sqlite3_errmsg(db_handle_) << std::endl;
        return;
    }

    sqlite3_exec(db_handle_, "BEGIN TRANSACTION;", 0, 0, 0);
    
    // Now we only insert each term once
    for (const auto& [term, frequency] : term_frequencies) {
        sqlite3_bind_text(stmt, 1, term.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 2, page_id);
        
        sqlite3_step(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_exec(db_handle_, "COMMIT;", 0, 0, 0);
    sqlite3_finalize(stmt);
}

std::vector<long long> Database::get_pages_for_term(const std::string& term) {
    std::vector<long long> page_ids;
    const char* sql = "SELECT page_id FROM inverted_index WHERE term = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing statement (get_pages_for_term): " << sqlite3_errmsg(db_handle_) << std::endl;
        return page_ids;
    }

    sqlite3_bind_text(stmt, 1, term.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        page_ids.push_back(sqlite3_column_int64(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return page_ids;
}

std::string Database::get_url_for_page_id(long long page_id) {
    std::string url;
    const char* sql = "SELECT url FROM pages WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing statement (get_url_for_page_id): " << sqlite3_errmsg(db_handle_) << std::endl;
        return url;
    }

    sqlite3_bind_int64(stmt, 1, page_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return url;
}