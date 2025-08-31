#include "HTTP_client.h"
#include <curl/curl.h> 
#include <iostream>

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
}

HttpClient::~HttpClient() {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }
    curl_global_cleanup();
}

std::string HttpClient::fetch(const std::string& url) {
    if (!curl_handle) {
        std::cerr << "Curl handle not initialized." << std::endl;
        return "";
    }

    std::string readBuffer;

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "MySimpleCrawler/1.0");

    CURLcode res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return ""; 
    }

    return readBuffer;
}