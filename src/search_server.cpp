// // /*
// //     i dont know how any of this works vibe coding to da moon
// // */

// #include "search/query_engine.h"
// #include <nlohmann/json.hpp>
// #include <iostream>
// #include <string>
// #include <memory>
// #include <vector>
// #include <thread>
// #include <chrono>

// // This tells the C++ compiler to treat the included header as a C-style library,
// // which prevents C++ name mangling and allows the linker to find the functions.
// extern "C" {
// #include <microhttpd.h>
// }

// // for convenience
// using json = nlohmann::json;

// // This is the main callback function that libmicrohttpd will call for every request.
// MHD_Result request_handler(void *cls,
//                          MHD_Connection *connection,
//                          const char *url,
//                          const char *method,
//                          const char *version,
//                          const char *upload_data,
//                          size_t *upload_data_size,
//                          void **con_cls) {

//     // Get the QueryEngine object that we passed in when starting the server.
//     QueryEngine* engine = static_cast<QueryEngine*>(cls);

//     std::string response_json_str;
//     int http_status = MHD_HTTP_OK;

//     // --- Routing and Logic ---
//     // We only care about GET requests to the /search endpoint.
//     if (std::string(method) == "GET" && std::string(url).rfind("/search", 0) == 0) {
        
//         // Get the value of the 'q' query parameter from the URL.
//         const char* query_c_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");

//         if (query_c_str != nullptr) {
//             std::string query = query_c_str;
            
//             // --- CORE LOGIC ---
//             // Use the QueryEngine to get the ranked search results.
//             std::vector<SearchResult> results = engine->search(query);
//             // --- END CORE LOGIC ---

//             // Convert the C++ vector of structs to a JSON array.
//             json json_results = json::array();
//             for (const auto& r : results) {
//                 json_results.push_back({
//                     {"url", r.url},
//                     {"score", r.score}
//                 });
//             }
//             response_json_str = json_results.dump();

//         } else {
//             http_status = MHD_HTTP_BAD_REQUEST;
//             response_json_str = R"({"status":"error", "message":"Query parameter 'q' is missing."})";
//         }
//     } else {
//         http_status = MHD_HTTP_NOT_FOUND;
//         response_json_str = R"({"status":"error", "message":"Endpoint not found. Use GET /search?q=..."})";
//     }

//     // --- Send Response ---
//     MHD_Response *response = MHD_create_response_from_buffer(response_json_str.length(),
//                                                              (void*)response_json_str.c_str(),
//                                                              MHD_RESPMEM_MUST_COPY);
//     // Add headers for CORS (to allow the HTML file to call it) and content type.
//     MHD_add_response_header(response, "Content-Type", "application/json");
//     MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    
//     int ret = MHD_queue_response(connection, http_status, response);
//     MHD_destroy_response(response);

//     return static_cast<MHD_Result>(ret);
// }


// int main() {
//     try {
//         Database db("search_engine.db");
//         QueryEngine engine(db);

//         unsigned short const port = 8080;
        
//         // Start the libmicrohttpd server (daemon).
//         MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION,
//                                               port,
//                                               NULL, NULL,
//                                               &request_handler, &engine, // Pass a pointer to our QueryEngine
//                                               MHD_OPTION_END);
//         if (daemon == NULL) {
//             std::cerr << "Failed to start the search service." << std::endl;
//             return 1;
//         }

//         std::cout << "Search Service started. Listening on http://localhost:" << port << std::endl;

//         // ✅ Keep the server alive indefinitely (instead of waiting for ENTER)
//         while (true) {
//             std::this_thread::sleep_for(std::chrono::hours(24));
//         }

//         // (Cleanup, probably never reached on Render)
//         MHD_stop_daemon(daemon);
//         std::cout << "Search service stopped." << std::endl;

//     } catch (const std::exception& e) {
//         std::cerr << "FATAL ERROR: " << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }


#include "indexer/indexer.h"
#include "search/query_engine.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

extern "C" {
#include <microhttpd.h>
}

using json = nlohmann::json;

// A single context struct to hold pointers to BOTH our services.
struct ServerContext {
    Indexer* indexer;
    QueryEngine* query_engine;
};

// Struct for handling POST request bodies
struct PostStatus {
    std::string body;
};

// A single, unified request handler that acts as a router.
MHD_Result request_handler(void *cls,
                         MHD_Connection *connection,
                         const char *url,
                         const char *method,
                         const char *version,
                         const char *upload_data,
                         size_t *upload_data_size,
                         void **con_cls) {

    ServerContext* context = static_cast<ServerContext*>(cls);
    std::string response_str;
    int http_status = MHD_HTTP_OK;

    // --- ROUTING LOGIC ---

    if (std::string(method) == "POST" && std::string(url) == "/crawl") {
        // --- INDEXER ENDPOINT LOGIC ---
        // (This part is mostly the same as your old indexer_server.cpp)
        PostStatus* post_status = static_cast<PostStatus*>(*con_cls);
        if (post_status == nullptr) { // First call for this connection
            *con_cls = new PostStatus();
            return MHD_YES;
        }
        if (*upload_data_size != 0) { // Accumulate body data
            post_status->body.append(upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }
        
        try {
            json request_body = json::parse(post_status->body);
            std::string seed_url = request_body.at("url");
            if (context->indexer->start_crawl(seed_url)) {
                response_str = R"({"status":"ok", "message":"Crawl initiated."})";
            } else {
                http_status = MHD_HTTP_CONFLICT;
                response_str = R"({"status":"error", "message":"A crawl is already in progress."})";
            }
        } catch (const json::exception& e) {
            http_status = MHD_HTTP_BAD_REQUEST;
            response_str = R"({"status":"error", "message":"Invalid JSON body."})";
        }
        delete post_status;
        *con_cls = nullptr;

    } else if (std::string(method) == "GET" && std::string(url).rfind("/search", 0) == 0) {
        // --- SEARCH ENDPOINT LOGIC ---
        // (This part is mostly the same as your old search_server.cpp)
        const char* query_c_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");
        if (query_c_str) {
            std::vector<SearchResult> results = context->query_engine->search(query_c_str);
            json json_results = json::array();
            for (const auto& r : results) {
                json_results.push_back({{"url", r.url}, {"score", r.score}});
            }
            response_str = json_results.dump();
        } else {
            http_status = MHD_HTTP_BAD_REQUEST;
            response_str = R"({"status":"error", "message":"Query parameter 'q' is missing."})";
        }

    } else {
        // --- NOT FOUND (DEFAULT) ---
        http_status = MHD_HTTP_NOT_FOUND;
        response_str = R"({"status":"error", "message":"Endpoint not found."})";
    }

    // --- Send Response ---
    MHD_Response *response = MHD_create_response_from_buffer(response_str.length(),
                                                             (void*)response_str.c_str(), MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(connection, http_status, response);
    MHD_destroy_response(response);
    return static_cast<MHD_Result>(ret);
}


int main() {
    // Get the port from an environment variable, which is standard for deployment platforms.
    // Default to 8080 for local development.
    unsigned short const port = 8080;

    try {
        // Create all the core logic objects.
        Database db("search_engine.db");
        Indexer indexer;
        QueryEngine query_engine(db);

        // Put pointers to our services in a context struct to pass to the handler.
        ServerContext context = {&indexer, &query_engine};

        MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION,
                                              port, NULL, NULL,
                                              &request_handler, &context, // Pass the context pointer
                                              MHD_OPTION_END);
        if (daemon == NULL) {
            std::cerr << "Failed to start the server." << std::endl;
            return 1;
        }

        std::cout << "Tesseract Unified Server started. Listening on http://localhost:" << port << std::endl;
        std::cout << "Endpoints:" << std::endl;
        std::cout << "  POST /crawl" << std::endl;
        std::cout << "  GET /search?q=..." << std::endl;
        std::cout << "Press ENTER to quit." << std::endl;
        
        std::cin.get();

        MHD_stop_daemon(daemon);
        std::cout << "Server stopped." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}