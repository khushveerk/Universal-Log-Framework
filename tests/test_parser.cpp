#include "log_processor.hpp"
#include <iostream>
#include <cassert>

void testJSONParsingAndRedaction() {
    std::string sample_json = R"({
        "timestamp": "2026-10-12T12:00:00Z",
        "level": "ERROR",
        "service": "payment-service",
        "message": "Failed transaction for user test@example.com from IP 192.168.1.50"
    })";

    CanonicalEvent event = LogProcessor::parseJSONLog(sample_json, "src-file-01");

    assert(event.processing_status == "SUCCESS");
    assert(event.severity == "ERROR");
    assert(event.service_name == "payment-service");
    assert(event.pii_detected == true);
    
    // Verify PII is masked properly
    assert(event.message.find("test@example.com") == std::string::npos);
    assert(event.message.find("[REDACTED_EMAIL]") != std::string::npos);
    assert(event.message.find("[REDACTED_IP]") != std::string::npos);

    std::cout << "[PASS] testJSONParsingAndRedaction passed!\n";
}

int main() {
    std::cout << "Running Phase 1 Parser & Redaction Tests...\n";
    testJSONParsingAndRedaction();
    std::cout << "All Phase 1 tests passed successfully!\n";
    return 0;
}