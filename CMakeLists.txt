cmake_minimum_required(VERSION 3.10)
project(ChoomSearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)

find_path(GUMBO_INCLUDE_DIR NAMES gumbo.h)
find_library(GUMBO_LIBRARY NAMES gumbo)
if(NOT GUMBO_INCLUDE_DIR OR NOT GUMBO_LIBRARY)
    message(FATAL_ERROR "Gumbo library not found. Please ensure it is installed via pacman.")
endif()

find_library(SQLITE3_LIBRARY NAMES sqlite3)
if(NOT SQLITE3_LIBRARY)
    message(FATAL_ERROR "SQLite3 library file not found. Please ensure it is installed via pacman.")
endif()

find_path(LIBMICROHTTPD_INCLUDE_DIR NAMES microhttpd.h)
find_library(LIBMICROHTTPD_LIBRARY NAMES microhttpd)
if(NOT LIBMICROHTTPD_INCLUDE_DIR OR NOT LIBMICROHTTPD_LIBRARY)
    message(FATAL_ERROR "Libmicrohttpd library not found. Please ensure it is installed via pacman.")
endif()

set(SHARED_SOURCES
    src/database/database.cpp
)

add_executable(indexer_server
    src/indexer_server.cpp
    src/indexer/indexer.cpp
    src/crawler/crawler.cpp
    src/network/HTTP_client.cpp
    src/parsing/HTML_parser.cpp
    src/concurrency/concurrent_queue.cpp
    ${SHARED_SOURCES}
)

add_executable(search_server
    src/search_server.cpp
    src/search/query_engine.cpp
    src/search/retriever.cpp
    src/search/ranker.cpp
    ${SHARED_SOURCES}
)

target_include_directories(indexer_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(search_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(indexer_server PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${GUMBO_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    ${LIBMICROHTTPD_INCLUDE_DIR}
)

target_include_directories(search_server PRIVATE
    ${SQLITE3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    ${LIBMICROHTTPD_INCLUDE_DIR}
)

target_link_libraries(indexer_server PRIVATE
    ${CURL_LIBRARIES}
    ${GUMBO_LIBRARY}
    ${SQLITE3_LIBRARY}
    Threads::Threads
    nlohmann_json::nlohmann_json
    ${LIBMICROHTTPD_LIBRARY}          
)

target_link_libraries(search_server PRIVATE
    ${SQLITE3_LIBRARY}
    nlohmann_json::nlohmann_json
    ${LIBMICROHTTPD_LIBRARY}         
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})