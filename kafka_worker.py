import json
import time
from kafka import KafkaConsumer
import re

def start_kafka_consumer(inverted_index_ref, storage_ref):
    print("[Kafka Worker] Connecting to local Kafka broker at localhost:9092...")
    
    # Retry loop in case Kafka is still initializing its ports
    consumer = None
    for attempt in range(5):
        try:
            consumer = KafkaConsumer(
                'log-events-topic',
                bootstrap_servers=['localhost:9092'],
                auto_offset_reset='earliest',
                enable_auto_commit=True,
                group_id='log-processing-group',
                value_deserializer=lambda x: json.loads(x.decode('utf-8'))
            )
            print("[Kafka Worker] Successfully connected to Kafka topic: log-events-topic")
            break
        except Exception as e:
            print(f"[Kafka Worker] Connection attempt {attempt+1} failed: {e}. Retrying in 3 seconds...")
            time.sleep(3)
            
    if not consumer:
        print("[Kafka Worker Error] Could not connect to Kafka. Running without background consumer.")
        return

    # Consume messages from Kafka stream
    for message in consumer:
        try:
            event = message.value
            event_id = event["event_id"]
            message_text = event["message"]
            
            # Store event body in storage
            storage_ref[event_id] = event
            
            # Index into inverted index
            words = re.findall(r'\b\w+\b', message_text.lower())
            for word in words:
                if word not in inverted_index_ref:
                    inverted_index_ref[word] = set()
                inverted_index_ref[word].add(event_id)
                
            print(f"[Kafka Worker] Processed and indexed event ID: {event_id}")
        except Exception as ex:
            print(f"[Kafka Worker Error] Error processing message: {ex}")