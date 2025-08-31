/*
    i dont know how any of this works vibe coding to da moon
*/

#include "search/query_engine.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

// This tells the C++ compiler to treat the included header as a C-style library,
// which prevents C++ name mangling and allows the linker to find the functions.
extern "C" {
#include <microhttpd.h>
}

// for convenience
using json = nlohmann::json;

// This is the main callback function that libmicrohttpd will call for every request.
MHD_Result request_handler(void *cls,
                         MHD_Connection *connection,
                         const char *url,
                         const char *method,
                         const char *version,
                         const char *upload_data,
                         size_t *upload_data_size,
                         void **con_cls) {

    // Get the QueryEngine object that we passed in when starting the server.
    QueryEngine* engine = static_cast<QueryEngine*>(cls);

    std::string response_json_str;
    int http_status = MHD_HTTP_OK;

    // --- Routing and Logic ---
    // We only care about GET requests to the /search endpoint.
    if (std::string(method) == "GET" && std::string(url).rfind("/search", 0) == 0) {
        
        // Get the value of the 'q' query parameter from the URL.
        const char* query_c_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");

        if (query_c_str != nullptr) {
            std::string query = query_c_str;
            
            // --- CORE LOGIC ---
            // Use the QueryEngine to get the ranked search results.
            std::vector<SearchResult> results = engine->search(query);
            // --- END CORE LOGIC ---

            // Convert the C++ vector of structs to a JSON array.
            json json_results = json::array();
            for (const auto& r : results) {
                json_results.push_back({
                    {"url", r.url},
                    {"score", r.score}
                });
            }
            response_json_str = json_results.dump();

        } else {
            http_status = MHD_HTTP_BAD_REQUEST;
            response_json_str = R"({"status":"error", "message":"Query parameter 'q' is missing."})";
        }
    } else {
        http_status = MHD_HTTP_NOT_FOUND;
        response_json_str = R"({"status":"error", "message":"Endpoint not found. Use GET /search?q=..."})";
    }

    // --- Send Response ---
    MHD_Response *response = MHD_create_response_from_buffer(response_json_str.length(),
                                                             (void*)response_json_str.c_str(),
                                                             MHD_RESPMEM_MUST_COPY);
    // Add headers for CORS (to allow the HTML file to call it) and content type.
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    
    int ret = MHD_queue_response(connection, http_status, response);
    MHD_destroy_response(response);

    return static_cast<MHD_Result>(ret);
}


int main() {
    // Create the Database and QueryEngine objects.
    try {
        Database db("search_engine.db");
        QueryEngine engine(db);

        unsigned short const port = 8080;
        
        // Start the libmicrohttpd server (daemon).
        MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_THREAD_PER_CONNECTION,
                                              port,
                                              NULL, NULL,
                                              &request_handler, &engine, // Pass a pointer to our QueryEngine
                                              MHD_OPTION_END);
        if (daemon == NULL) {
            std::cerr << "Failed to start the search service." << std::endl;
            return 1;
        }

        std::cout << "Search Service started. Listening on http://localhost:" << port << std::endl;
        while (true) {
    std::this_thread::sleep_for(std::chrono::hours(24));
}


        MHD_stop_daemon(daemon);
        std::cout << "Search service stopped." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}