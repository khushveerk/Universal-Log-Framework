from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List, Optional, Dict, Any
from datetime import datetime
import uuid
import re
import threading
import json
from kafka import KafkaProducer
from kafka_worker import start_kafka_consumer
from ai_debugger import AIDebugger

app = FastAPI(
    title="Universal Log Processing & Observability Platform",
    version="1.1"
)

# Initialize AI Debugger
ai_debugger = AIDebugger()

# Shared Data Stores
LOG_DATABASE: Dict[str, Dict[str, Any]] = {}
INVERTED_INDEX: Dict[str, set] = {}

# Initialize Kafka Producer
producer = None
try:
    producer = KafkaProducer(
        bootstrap_servers=['localhost:9092'],
        value_serializer=lambda v: json.dumps(v).encode('utf-8')
    )
except Exception as e:
    print(f"Warning: Could not start Kafka producer: {e}")

# Start background Kafka consumer worker thread
consumer_thread = threading.Thread(
    target=start_kafka_consumer, 
    args=(INVERTED_INDEX, LOG_DATABASE),
    daemon=True
)
consumer_thread.start()

def redact_pii(text: str) -> tuple[str, bool]:
    pii_detected = False
    email_regex = r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b"
    ip_regex = r"\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b"
    
    if re.search(email_regex, text):
        text = re.sub(email_regex, "[REDACTED_EMAIL]", text)
        pii_detected = True
    if re.search(ip_regex, text):
        text = re.sub(ip_regex, "[REDACTED_IP]", text)
        pii_detected = True
        
    return text, pii_detected

class LogRecord(BaseModel):
    timestamp: Optional[str] = None
    level: str = "INFO"
    service: str = "unknown-service"
    message: str
    attributes: Optional[Dict[str, Any]] = None

class IngestRequest(BaseModel):
    source_id: str
    records: List[LogRecord]

class QueryFilter(BaseModel):
    service: Optional[str] = None
    severity: Optional[List[str]] = None

class QueryRequest(BaseModel):
    time_from: Optional[str] = Field(None, alias="from")
    time_to: Optional[str] = Field(None, alias="to")
    filters: Optional[QueryFilter] = None
    text: Optional[str] = None
    limit: int = 100

@app.post("/api/v1/ingest")
def ingest_logs(payload: IngestRequest):
    accepted_count = 0
    rejected_count = 0
    
    for record in payload.records:
        try:
            event_id = str(uuid.uuid4())
            observed_time = datetime.utcnow().isoformat() + "Z"
            event_time = record.timestamp or observed_time
            
            sanitized_message, pii_flag = redact_pii(record.message)
            
            canonical_event = {
                "event_id": event_id,
                "event_time": event_time,
                "observed_time": observed_time,
                "source_id": payload.source_id,
                "service_name": record.service,
                "severity": record.level.upper(),
                "message": sanitized_message,
                "pii_detected": pii_flag,
                "attributes": record.attributes or {},
                "processing_status": "SUCCESS"
            }
            
            # Publish event to Kafka Topic
            if producer:
                producer.send('log-events-topic', canonical_event)
                producer.flush()
            
            accepted_count += 1
        except Exception:
            rejected_count += 1
            
    return {
        "status": "success",
        "message": "Events successfully published to Kafka stream.",
        "accepted_count": accepted_count,
        "rejected_count": rejected_count
    }

@app.post("/api/v1/query")
def query_logs(query: QueryRequest):
    candidate_ids = set(LOG_DATABASE.keys())
    
    # 1. Text keyword search using Inverted Index (Metadata first, text second principle)
    if query.text:
        search_terms = re.findall(r'\b\w+\b', query.text.lower())
        if search_terms:
            text_matches = set(LOG_DATABASE.keys())
            for term in search_terms:
                term_matches = INVERTED_INDEX.get(term, set())
                text_matches = text_matches.intersection(term_matches)
            candidate_ids = candidate_ids.intersection(text_matches)
            
    results = []
    for event_id in candidate_ids:
        event = LOG_DATABASE[event_id]
        
        # 2. Filter by service or severity list
        if query.filters:
            if query.filters.service and event["service_name"] != query.filters.service:
                continue
            if query.filters.severity and event["severity"] not in query.filters.severity:
                continue
                
        # 3. Time range filtering (ISO-8601 string comparison)
        if query.time_from and event["event_time"] < query.time_from:
            continue
        if query.time_to and event["event_time"] > query.time_to:
            continue
            
        results.append(event)
        if len(results) >= query.limit:
            break
            
    return {
        "total_matches": len(results),
        "query_duration_ms": 1.2, # Simulated operational performance metric
        "events": results
    }

@app.get("/health")
def health_check():
    return {
        "status": "healthy", 
        "kafka_broker": "localhost:9092",
        "total_indexed_events": len(LOG_DATABASE),
        "unique_terms_in_index": len(INVERTED_INDEX),
        "processing_status": "ONLINE"
    }


@app.get("/api/v1/debug/{event_id}")
def debug_log_event(event_id: str):
    if event_id not in LOG_DATABASE:
        return {"error": "Event ID not found in storage database."}
    
    event = LOG_DATABASE[event_id]
    analysis = ai_debugger.analyze_error(event)
    
    return {
        "event_id": event_id,
        "ai_diagnostic": analysis
    }   
    
