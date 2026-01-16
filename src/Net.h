#include <SDK/util/Logger.hpp>
#include <string>
#include <optional>
#include <cstring>
#include <curl/curl.h>

struct ParsedURL {
    std::string scheme;
    std::string host;
    std::string path;
    int port;
};

class NetManager {
public:
    NetManager() {
        curl = curl_easy_init();
        if (!curl) Logger::error("Failed to initialize CURL");
    }

    ~NetManager() {
        if (curl) curl_easy_cleanup(curl);
    }

    struct Result {
        long status = 0;
        std::string body;
        std::string_view content_type;
        bool ok() const { return status >= 200 && status < 300; }
    };

    Result Get(const std::string& url) {
        Result result;
        std::string buffer;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "ResourceDragon/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            Logger::error("CURL error: {}", curl_easy_strerror(res));

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);

        char* content_type = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
        if (content_type) result.content_type = content_type;

        result.body = std::move(buffer);
        return result;
    }

    static std::optional<ParsedURL> parse_url(const std::string& url) {
        ParsedURL out;
        out.port = 0;

        auto scheme_end = url.find("://");
        if (scheme_end == std::string::npos) return std::nullopt;
        out.scheme = url.substr(0, scheme_end);

        auto host_start = scheme_end + 3;
        auto path_start = url.find('/', host_start);

        std::string host_port;
        if (path_start == std::string::npos) {
            host_port = url.substr(host_start);
            out.path = "/";
        } else {
            host_port = url.substr(host_start, path_start - host_start);
            out.path = url.substr(path_start);
        }

        // split host and port
        auto colon = host_port.find(':');
        if (colon != std::string::npos) {
            out.host = host_port.substr(0, colon);
            out.port = std::stoi(host_port.substr(colon + 1));
        } else {
            out.host = host_port;
            out.port = (out.scheme == "https") ? 443 : 80;
        }

        return out;
    }

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        std::string* s = (std::string*)userp;
        s->append((char*)contents, total);
        return total;
    }

    CURL* curl = nullptr;
};
