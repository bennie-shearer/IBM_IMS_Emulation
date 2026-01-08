// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Storage Example
// Version: 3.6.3
// =============================================================================

#include <iostream>
#include "ims/common/types.hpp"
#include "ims/common/storage.hpp"
#include "ims/common/string_utils.hpp"

using namespace ims;
using namespace ims::storage;

int main() {
    std::cout << "=================================================\n";
    std::cout << "IBM IMS (Information Management System) Emulation Enterprise - Storage Example\n";
    std::cout << "Version 3.6.3\n";
    std::cout << "=================================================\n\n";
    
    // Memory storage demo
    std::cout << "=== Memory Storage Demo ===\n\n";
    
    MemoryStorage storage(1024 * 1024);  // 1MB
    
    String data = "Hello from IBM IMS (Information Management System) Emulation Enterprise!";
    storage.write(data.data(), data.size(), 0);
    std::cout << "Wrote: \"" << data << "\"\n";
    
    char buffer[100] = {0};
    storage.read(buffer, data.size(), 0);
    std::cout << "Read:  \"" << buffer << "\"\n";
    
    std::cout << "\nStorage size: " << strings::format_bytes(storage.size()) << "\n";
    
    auto stats = storage.stats();
    std::cout << "Read ops: " << stats.read_ops << "\n";
    std::cout << "Write ops: " << stats.write_ops << "\n";
    
    std::cout << "\n=================================================\n";
    std::cout << "Storage example completed successfully!\n";
    std::cout << "=================================================\n";
    
    return 0;
}
