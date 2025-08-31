/*
    i dont know what is this code its vibe coded i will look into this
*/
#include "indexer/indexer.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

// This tells the C++ compiler to treat the included header as a C-style library,
// which prevents C++ name mangling and allows the linker to find the functions.
extern "C" {
#include <microhttpd.h>
}

// for convenience
using json = nlohmann::json;

// This struct will hold the state for each POST request.
struct PostStatus {
    std::string body;
};

// This is the main callback function that libmicrohttpd will call for every request.
MHD_Result request_handler(void *cls,
                         MHD_Connection *connection,
                         const char *url,
                         const char *method,
                         const char *version,
                         const char *upload_data,
                         size_t *upload_data_size,
                         void **con_cls) {

    // Get the SINGLE Indexer object (passed from main).
    Indexer* indexer = static_cast<Indexer*>(cls);
    
    // Handle POST request body accumulation
    if (*con_cls == nullptr) {
        PostStatus* post_status = new PostStatus();
        *con_cls = post_status;
        return MHD_YES;
    }
    
    PostStatus* post_status = static_cast<PostStatus*>(*con_cls);

    if (*upload_data_size != 0) {
        post_status->body.append(upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }
    
    // --- Request processing logic ---
    std::string response_json_str;
    int http_status = MHD_HTTP_OK;

    if (std::string(method) == "POST" && std::string(url) == "/crawl") {
        try {
            json request_body = json::parse(post_status->body);
            std::string seed_url = request_body.at("url");

            if (seed_url.empty()) {
                http_status = MHD_HTTP_BAD_REQUEST;
                response_json_str = R"({"status":"error", "message":"Seed URL cannot be empty."})";
            } else {
                if (indexer->start_crawl(seed_url)) {
                    response_json_str = R"({"status":"ok", "message":"Crawl initiated."})";
                } else {
                    http_status = MHD_HTTP_CONFLICT;
                    response_json_str = R"({"status":"error", "message":"A crawl is already in progress."})";
                }
            }
        } catch (const json::exception& e) {
            http_status = MHD_HTTP_BAD_REQUEST;
            response_json_str = R"({"status":"error", "message":"Invalid JSON body."})";
        }
    } else {
        http_status = MHD_HTTP_NOT_FOUND;
        response_json_str = R"({"status":"error", "message":"Endpoint not found. Use POST /crawl"})";
    }

    // --- Send Response ---
    MHD_Response *response = MHD_create_response_from_buffer(response_json_str.length(),
                                                             (void*)response_json_str.c_str(),
                                                             MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "application/json");
    
    int ret = MHD_queue_response(connection, http_status, response);
    MHD_destroy_response(response);

    // Clean up the PostStatus object for this connection.
    delete post_status;
    *con_cls = nullptr;

    return static_cast<MHD_Result>(ret);
}


int main() {
    // Create ONE Indexer object
    Indexer indexer;

    unsigned short const port = 8081;
    
    // Start the libmicrohttpd server (daemon).
    MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION,
                                          port,
                                          NULL, NULL,
                                          &request_handler, &indexer,
                                          MHD_OPTION_END);
    if (daemon == NULL) {
        std::cerr << "Failed to start the indexer service." << std::endl;
        return 1;
    }

    std::cout << "Indexer Service started. Listening on http://localhost:" << port << std::endl;

    // ✅ Keep running forever (instead of waiting for ENTER)
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(24));
    }

    // (Probably never reached on Render, but safe cleanup)
    MHD_stop_daemon(daemon);
    std::cout << "Indexer service stopped." << std::endl;

    return 0;
}
