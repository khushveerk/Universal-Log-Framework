import requests
import time

BASE_URL = "http://127.0.0.1:8000"

def test_end_to_end_pipeline():
    print("Testing End-to-End Log Pipeline with Kafka & Inverted Index...")

    # 1. Ingest sample logs containing PII (email/IP)
    ingest_payload = {
        "source_id": "test-source-cli",
        "records": [
            {
                "timestamp": "2026-10-31T12:00:00Z",
                "level": "ERROR",
                "service": "auth-service",
                "message": "Authentication failed for user admin@test.com from IP 10.0.0.51"
            },
            {
                "timestamp": "2026-10-31T12:05:00Z",
                "level": "INFO",
                "service": "payment-service",
                "message": "Payment processed successfully for order #9928"
            }
        ]
    }

    response = requests.post(f"{BASE_URL}/api/v1/ingest", json=ingest_payload)
    assert response.status_code == 200
    print("[PASS] Ingestion request accepted by API producer!")

    # Wait a moment for Kafka consumer worker to pick up and index the message
    time.sleep(2)

    # 2. Query logs using text keyword search ("authentication") and severity filter
    query_payload = {
        "filters": {
            "severity": ["ERROR"]
        },
        "text": "authentication"
    }

    query_response = requests.post(f"{BASE_URL}/api/v1/query", json=query_payload)
    assert query_response.status_code == 200
    data = query_response.json()
    
    print(f"Query returned {data['total_matches']} match(es).")
    assert data["total_matches"] >= 1
    
    matched_event = data["events"][0]
    print(f"Matched Event Message: {matched_event['message']}")

    # Verify PII Redaction worked through Kafka
    assert "admin@test.com" not in matched_event["message"]
    assert "[REDACTED_EMAIL]" in matched_event["message"]
    assert "[REDACTED_IP]" in matched_event["message"]
    print("[PASS] PII Redaction verified through Kafka streaming consumer!")

    print("All Phase 4 Query & Pipeline Integration tests passed successfully!")

if __name__ == "__main__":
    test_end_to_end_pipeline()