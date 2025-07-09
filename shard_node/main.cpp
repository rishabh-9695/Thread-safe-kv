#include <iostream>
#include "kvstore.hpp"

int main() {
    KVStore kvstore; // Create an instance of KVStore
    kvstore.put("key1", "value1"); // Example usage of KVStore
    std::cout << "Stored key1 with value: " << kvstore.get("key1") << std::endl; // Retrieve and print the value

    kvstore.remove("key1"); // Remove the key
    std::cout << "Removed key1, current value: " << kvstore.get("key1") << std::endl; // Should print an empty string
    std::cin.get(); // Wait for user input before exiting
    return 0;
}