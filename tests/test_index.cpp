#include "inverted_index.hpp"
#include <iostream>
#include <cassert>

void testInvertedIndexOperations() {
    InvertedIndex index;

    // Add sample documents (mocking parsed log events)
    index.addDocument("evt-001", "Payment gateway timeout error during transaction");
    index.addDocument("evt-002", "User authentication failed for admin account");
    index.addDocument("evt-003", "Payment gateway connection reset error");

    // Test single keyword search
    auto payment_results = index.searchKeyword("payment");
    assert(payment_results.size() == 2);
    assert(payment_results.count("evt-001") == 1);
    assert(payment_results.count("evt-003") == 1);

    // Test AND search intersection
    auto and_results = index.searchAND({"payment", "error"});
    assert(and_results.size() == 2);
    assert(and_results.count("evt-001") == 1);
    assert(and_results.count("evt-003") == 1);

    auto specific_and = index.searchAND({"payment", "timeout"});
    assert(specific_and.size() == 1);
    assert(specific_and.count("evt-001") == 1);

    std::cout << "[PASS] testInvertedIndexOperations passed!\n";
}

int main() {
    std::cout << "Running Phase 2 Inverted Index Tests...\n";
    testInvertedIndexOperations();
    std::cout << "All Phase 2 Index tests passed successfully!\n";
    return 0;
}