#include "http_client.h"
#include <spdlog/spdlog.h>

HttpClient::HttpClient(std::chrono::seconds timeout) : timeout_(timeout) {}

HttpResponse HttpClient::get(const std::string& url, const std::vector<cpr::Pair>& params) {
    try {
        cpr::Session session;
        session.SetUrl(cpr::Url{url});
        session.SetTimeout(timeout_);
        session.SetOption(getSslOptions());

        if (!params.empty()) {
            std::string full_url = url;
            full_url += "?";
            for (size_t i = 0; i < params.size(); ++i) {
                if (i > 0) full_url += "&";
                full_url += params[i].key + "=" + params[i].value;
            }
            session.SetUrl(cpr::Url{full_url});
        }

        auto response = session.Get();

        HttpResponse result;
        result.success = response.status_code >= 200 && response.status_code < 300;
        result.status_code = static_cast<int>(response.status_code);
        result.body = response.text;
        result.error = response.error.message;
        return result;
    } catch (const std::exception& e) {
        spdlog::error("HTTP GET ошибка: {}", e.what());
        HttpResponse result;
        result.success = false;
        result.status_code = 0;
        result.error = e.what();
        return result;
    }
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, const std::string& content_type) {
    try {
        cpr::Session session;
        session.SetUrl(cpr::Url{url});
        session.SetHeader(cpr::Header{{"Content-Type", content_type}});
        session.SetBody(cpr::Body{body});
        session.SetTimeout(timeout_);
        session.SetOption(getSslOptions());

        auto response = session.Post();

        HttpResponse result;
        result.success = response.status_code >= 200 && response.status_code < 300;
        result.status_code = static_cast<int>(response.status_code);
        result.body = response.text;
        result.error = response.error.message;
        return result;
    } catch (const std::exception& e) {
        spdlog::error("HTTP POST ошибка: {}", e.what());
        HttpResponse result;
        result.success = false;
        result.status_code = 0;
        result.error = e.what();
        return result;
    }
}

HttpResponse HttpClient::postFiles(const std::string& url, const std::vector<cpr::Pair>& fields, const std::vector<FileData>& files) {
    try {
        cpr::Multipart multipart{};

        for (const auto& field : fields) {
            multipart.parts.push_back({field.key, field.value});
        }
        for (const auto& file : files) {
            multipart.parts.push_back({file.key, cpr::File{file.filepath}});
        }

        cpr::Session session;
        session.SetUrl(cpr::Url{url});
        session.SetMultipart(multipart);
        session.SetTimeout(timeout_);
        session.SetOption(getSslOptions());

        auto response = session.Post();

        HttpResponse result;
        result.success = response.status_code >= 200 && response.status_code < 300;
        result.status_code = static_cast<int>(response.status_code);
        result.body = response.text;
        result.error = response.error.message;
        return result;
    } catch (const std::exception& e) {
        spdlog::error("HTTP POST файлы ошибка: {}", e.what());
        HttpResponse result;
        result.success = false;
        result.status_code = 0;
        result.error = e.what();
        return result;
    }
}

cpr::SslOptions HttpClient::getSslOptions() const {
    if (verify_ssl_) {
        return cpr::Ssl(cpr::ssl::VerifyPeer{true}, cpr::ssl::VerifyHost{true});
    }
    return cpr::Ssl(cpr::ssl::VerifyPeer{false}, cpr::ssl::VerifyHost{false});
}