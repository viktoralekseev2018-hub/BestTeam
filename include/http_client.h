#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cpr/cpr.h>

struct HttpResponse {
    bool success;
    int status_code;
    std::string body;
    std::string error;
};

struct FileData {
    std::string key;
    std::string filepath;
};

class HttpClient {
public:
    explicit HttpClient(std::chrono::seconds timeout = std::chrono::seconds(30));

    HttpResponse get(const std::string& url, const std::vector<cpr::Pair>& params = {});
    HttpResponse post(const std::string& url, const std::string& body, 
                      const std::string& content_type = "application/json");
    HttpResponse postFiles(const std::string& url, const std::vector<cpr::Pair>& fields, 
                           const std::vector<FileData>& files);

    void setTimeout(std::chrono::seconds timeout) { timeout_ = timeout; }
    void setVerifySsl(bool verify) { verify_ssl_ = verify; }

private:
    std::chrono::seconds timeout_;
    bool verify_ssl_{true};

    cpr::SslOptions getSslOptions() const;
};