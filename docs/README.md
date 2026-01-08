# IMS Emulation
Version 3.6.2 | January 2026

---

## Overview

IBM IMS Emulation Enterprise provides a comprehensive emulation of IBM mainframe data management systems, enabling development, testing, and training without requiring access to actual mainframe hardware.

### Key Features

- **IMS Database Emulation**: Full DL/I call support (GU, GN, GNP, ISRT, DLET, REPL)
- **VSAM Support**: KSDS, ESDS, RRDS, and LDS dataset organizations
- **Master Catalog**: z/OS-compatible catalog management
- **GDG Support**: Generation Data Group management
- **DFSMShsm**: Storage management and migration simulation
- **Security Framework**: RACF-compatible security model

### Requirements

- C++20 compatible compiler:
  - GCC 11+
  - Clang 14+
  - MSVC 2022+
  - MinGW-w64 with GCC 11+
- CMake 3.20+
- No external dependencies

---

## Quick Start

### Building on Linux/macOS

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Building on Windows (MSVC)

```batch
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Building on Windows (MinGW)

```batch
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

### Running the Demo

```bash
./bin/ims-console-demo
```

---

## Project Structure

```
IMS-Emulation-Enterprise-v3.6.2/
|---- CMakeLists.txt          # Main build configuration
|---- cmake/                  # CMake support files
|---- libs/                   # Core libraries
|   |---- common/            # Common types and utilities
|   |---- security/          # Security framework
|   |---- master-catalog/    # Catalog management
|   |---- vsam/              # VSAM emulation
|   |---- dfsmshsm/          # Storage management
|   |---- ims-core/          # IMS database emulation
|   \---- gdg/               # Generation Data Groups
|---- apps/                   # Applications
|   \---- console-demo/      # Demo application
|---- examples/              # Example programs
|---- tests/                 # Unit tests
\---- docs/                  # Documentation
```

---

## Documentation

- [BACKGROUND.md](BACKGROUND.md) - History and design philosophy
- [API_REFERENCE.md](API_REFERENCE.md) - Complete API documentation
- [BUILD_NOTES.md](BUILD_NOTES.md) - Platform-specific build instructions
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [IMPROVEMENTS.md](IMPROVEMENTS.md) - Recent enhancements

---

## Basic Usage Example

```cpp
#include <ims/ims/ims_types.hpp>
#include <ims/catalog/master_catalog.hpp>
#include <ims/vsam/vsam_types.hpp>

int main() {
    using namespace ims;
    using namespace ims::imsdb;
    using namespace ims::catalog;
    
    // Create a catalog entry
    CatalogEntry entry;
    entry.name = "USER.DATABASE.CUSTOMERS";
    entry.dsorg = DatasetOrganization::VSAM;
    entry.vsam_type = VsamType::KSDS;
    entry.lrecl = 500;
    entry.keylen = 10;
    
    // Create database definition
    DatabaseDefinition db;
    db.name = "CUSTDB";
    db.type = DatabaseType::HIDAM;
    
    // Add segment definitions
    SegmentDefinition root;
    root.name = "CUSTOMER";
    root.type = SegmentType::ROOT;
    root.length = 200;
    root.key_length = 10;
    db.segments.push_back(root);
    
    return 0;
}
```

---

## License

This project is licensed under the MIT License - see the [LICENSE.txt](LICENSE.txt) file for details.

Copyright (c) 2025 Bennie Shearer

---

## Author

Bennie Shearer (retired)

---

## Acknowledgments

Thanks to all my C++ mentors through the years.

Special thanks to:

| Organization | Website |
|--------------|---------|
| IMS by IBM | https://www.ibm.com/ |
| CLion by JetBrains s.r.o. | https://www.jetbrains.com/clion/ |
| Claude by Anthropic PBC | https://www.anthropic.com/ |
