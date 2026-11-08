#include "log_processor.hpp"
#include <regex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>

// Helper to generate a simple unique event ID (UUID-like placeholder)
std::string generateEventId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; ++i) {
        ss << dis(gen);
        if (i == 7 || i == 11 || i == 15 || i == 19) ss << "-";
    }
    return ss.str();
}

// Helper to get current UTC timestamp in ISO-8601
std::string getCurrentUTCTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

std::string LogProcessor::redactPII(const std::string& input, bool& out_pii_found) {
    std::string result = input;
    out_pii_found = false;

    // Regex for Email Addresses
    std::regex email_regex(R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b)");
    // Regex for IP Addresses
    std::regex ip_regex(R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)");
    // Regex for basic API keys / Bearer tokens
    std::regex token_regex(R"((bearer\s+|api[_-]key['"]?\s*[:=]\s*)[A-Za-z0-9\-\._~\+\/]+=*)", std::regex_constants::icase);

    if (std::regex_search(result, email_regex)) {
        result = std::regex_replace(result, email_regex, "[REDACTED_EMAIL]");
        out_pii_found = true;
    }
    if (std::regex_search(result, ip_regex)) {
        result = std::regex_replace(result, ip_regex, "[REDACTED_IP]");
        out_pii_found = true;
    }
    if (std::regex_search(result, token_regex)) {
        result = std::regex_replace(result, token_regex, "$1[REDACTED_SECRET]");
        out_pii_found = true;
    }

    return result;
}

CanonicalEvent LogProcessor::parseJSONLog(const std::string& raw_json, const std::string& source_id) {
    CanonicalEvent event;
    event.event_id = generateEventId();
    event.observed_time = getCurrentUTCTimestamp();
    event.source_id = source_id;

    try {
        auto j = nlohmann::json::parse(raw_json);
        
        event.event_time = j.value("timestamp", event.observed_time);
        event.service_name = j.value("service", "unknown-service");
        event.severity = j.value("level", "INFO");
        
        std::string raw_message = j.value("message", raw_json);
        event.message = redactPII(raw_message, event.pii_detected);

        // Capture remaining attributes
        for (auto& [key, val] : j.items()) {
            if (key != "timestamp" && key != "level" && key != "message" && key != "service") {
                event.attributes[key] = val;
            }
        }
        event.processing_status = "SUCCESS";
    } catch (const std::exception& e) {
        event.processing_status = "FAILED_PARSE";
        event.severity = "ERROR";
        event.message = "Failed to parse JSON log: " + std::string(e.what());
    }

    return event;
}

CanonicalEvent LogProcessor::parseTextLog(const std::string& raw_text, const std::string& source_id) {
    CanonicalEvent event;
    event.event_id = generateEventId();
    event.observed_time = getCurrentUTCTimestamp();
    event.source_id = source_id;
    event.event_time = event.observed_time; 

    // Simple regex pattern for basic Syslog-like text: [Timestamp] [Level] Message
    std::regex syslog_regex(R"(^(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?Z?)\s+([A-Z]+)\s+(.+)$)");
    std::smatch match;

    if (std::regex_match(raw_text, match, syslog_regex)) {
        event.event_time = match[1];
        event.severity = match[2];
        std::string raw_msg = match[3];
        event.message = redactPII(raw_msg, event.pii_detected);
        event.processing_status = "SUCCESS";
    } else {
        event.severity = "INFO";
        event.message = redactPII(raw_text, event.pii_detected);
        event.processing_status = "SUCCESS_UNSTRUCTURED";
    }

    return event;
}