#ifndef LOG_PROCESSOR_HPP
#define LOG_PROCESSOR_HPP

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

struct CanonicalEvent {
    std::string event_id;
    std::string event_time;
    std::string observed_time;
    std::string source_id;
    std::string service_name;
    std::string severity;
    std::string message;
    bool pii_detected = false;
    std::string processing_status = "PARSED";
    nlohmann::json attributes = nlohmann::json::object();
};

class LogProcessor {
public:
    // Redacts PII like emails, IP addresses, or tokens from a text string
    static std::string redactPII(const std::string& input, bool& out_pii_found);

    // Parses a raw JSON log line and normalizes it to our canonical schema
    static CanonicalEvent parseJSONLog(const std::string& raw_json, const std::string& source_id);

    // Parses a raw Syslog / plain text log line using regex
    static CanonicalEvent parseTextLog(const std::string& raw_text, const std::string& source_id);
};

#endif // LOG_PROCESSOR_HPP