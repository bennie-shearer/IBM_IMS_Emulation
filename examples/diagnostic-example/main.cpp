// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Diagnostic Example
// Version: 3.6.3
// =============================================================================

#include <iostream>
#include "ims/common/types.hpp"
#include "ims/common/diagnostics.hpp"
#include "ims/common/validation.hpp"
#include "ims/common/string_utils.hpp"
#include "ims/common/time_utils.hpp"

using namespace ims;
using namespace ims::diagnostics;
using namespace ims::validation;
using namespace ims::time;

int main() {
    std::cout << "=================================================\n";
    std::cout << "IBM IMS (Information Management System) Emulation Enterprise - Diagnostic Example\n";
    std::cout << "Version 3.6.3\n";
    std::cout << "=================================================\n\n";
    
    // Hex dump demo
    std::cout << "=== Hex Dump Demo ===\n\n";
    ByteBuffer data = {
        0x00, 0x01, 0x02, 0x03, 0x48, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21,
        0xFF, 0xFE, 0xFD, 0xFC, 0x00, 0x00, 0x00, 0x00
    };
    
    std::cout << HexDump::format(data) << "\n";
    
    // EBCDIC translation demo
    std::cout << "=== EBCDIC Translation Demo ===\n\n";
    String ascii_text = "HELLO WORLD";
    auto ebcdic = EbcdicTranslator::ascii_to_ebcdic(
        ByteBuffer(ascii_text.begin(), ascii_text.end()));
    
    std::cout << "ASCII:  " << ascii_text << "\n";
    std::cout << "EBCDIC: ";
    for (auto b : ebcdic) {
        std::cout << std::format("{:02X} ", b);
    }
    std::cout << "\n\n";
    
    // Validation demo
    std::cout << "=== Validation Demo ===\n\n";
    
    auto result1 = DatasetNameValidator::validate("USER.PROD.CUSTOMER.DATA");
    std::cout << "Dataset 'USER.PROD.CUSTOMER.DATA': " 
              << (result1 ? "Valid" : "Invalid") << "\n";
    
    auto result2 = DatasetNameValidator::validate("INVALID..NAME");
    std::cout << "Dataset 'INVALID..NAME': " 
              << (result2 ? "Valid" : "Invalid");
    if (!result2) {
        std::cout << " - " << result2.errors[0];
    }
    std::cout << "\n";
    
    auto result3 = VolumeSerialValidator::validate("PROD01");
    std::cout << "Volume 'PROD01': " 
              << (result3 ? "Valid" : "Invalid") << "\n\n";
    
    // Time utilities demo
    std::cout << "=== Time Utilities Demo ===\n\n";
    
    auto now_iso = TimestampFormatter::now_iso8601();
    auto now_mf = TimestampFormatter::now_mainframe();
    auto now_db2 = TimestampFormatter::now_db2();
    
    std::cout << "ISO 8601:   " << now_iso << "\n";
    std::cout << "Mainframe:  " << now_mf << "\n";
    std::cout << "DB2:        " << now_db2 << "\n\n";
    
    auto julian = JulianDate::today();
    std::cout << "Julian (YYDDD):   " << julian.to_yyddd() << "\n";
    std::cout << "Julian (YYYYDDD): " << julian.to_yyyyddd() << "\n\n";
    
    auto stck = Stck::now();
    std::cout << "STCK: " << stck.to_hex() << "\n\n";
    
    // Debug trace demo
    std::cout << "=== Debug Trace Demo ===\n\n";
    DebugTrace::enable(DebugTrace::DEBUG);
    
    IMS_TRACE_INFO("Application started");
    IMS_TRACE_DEBUG("Processing {} records", 100);
    IMS_TRACE_WARNING("Low memory condition detected");
    
    DebugTrace::disable();
    std::cout << "\n";
    
    // Diagnostic collector
    std::cout << "=== Diagnostic Info ===\n\n";
    global_diagnostics().log("System initialized");
    global_diagnostics().log("VSAM subsystem ready");
    global_diagnostics().log("IMS database connected");
    
    auto diag = global_diagnostics().collect("IMS-CORE");
    std::cout << "Component: " << diag.component << "\n";
    std::cout << "Version:   " << diag.version << "\n";
    std::cout << "Platform:  " << diag.platform << "\n";
    std::cout << "Compiler:  " << diag.compiler << "\n";
    std::cout << "Uptime:    " << diag.uptime_seconds << " seconds\n";
    
    std::cout << "\n=================================================\n";
    std::cout << "Diagnostic example completed successfully!\n";
    std::cout << "=================================================\n";
    
    return 0;
}
