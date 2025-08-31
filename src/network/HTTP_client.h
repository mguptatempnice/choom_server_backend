#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    std::string fetch(const std::string& url);

private:
    void* curl_handle;
};

#endif 