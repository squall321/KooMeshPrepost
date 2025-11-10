# Qt LGPL v3 Compliance Guide

## Overview

KooMeshPrepost uses Qt under the **LGPL v3 license**, which allows free use for both open-source and commercial applications, provided specific requirements are met.

## LGPL v3 Requirements

### 1. Dynamic Linking (Required)

**✓ IMPLEMENTED**: KooMeshPrepost uses Qt as a **dynamically linked shared library**.

- Qt libraries (.so/.dll/.dylib) are linked at runtime, not statically compiled
- Users can replace Qt libraries with different versions
- See [CMake Configuration](#cmake-configuration) for details

### 2. No Qt Modification

**✓ COMPLIANT**: KooMeshPrepost does NOT modify Qt source code.

- We use Qt as-is through its public API
- No patches or modifications to Qt itself
- All customization is done through Qt's documented extension mechanisms

### 3. User's Right to Relink

**✓ COMPLIANT**: Users can replace Qt libraries.

The LGPL requires that users can:
- Replace Qt shared libraries with their own versions
- Relink the application with different Qt versions (ABI compatible)
- Modify Qt and use their modified version

**How to replace Qt libraries:**

```bash
# Linux
export LD_LIBRARY_PATH=/path/to/custom/qt/lib:$LD_LIBRARY_PATH
./koomesh

# macOS
export DYLD_LIBRARY_PATH=/path/to/custom/qt/lib:$DYLD_LIBRARY_PATH
./koomesh

# Windows
# Place custom Qt DLLs in the same directory as koomesh.exe
# or add to PATH
```

### 4. License Notices

**✓ COMPLIANT**: All required notices are provided.

- Qt is licensed under LGPL v3 (see qt-licenses/LGPL_EXCEPTION.txt)
- KooMeshPrepost source code is available
- This compliance document is provided

### 5. Source Code Availability

**✓ COMPLIANT**: Full source code provided.

- KooMeshPrepost source: https://github.com/squall321/KooMeshPrepost
- Qt source code: https://www.qt.io/download-open-source

## CMake Configuration

Our CMakeLists.txt ensures LGPL compliance:

```cmake
# Find Qt (shared libraries only)
find_package(Qt6 COMPONENTS Core Widgets OpenGLWidgets)

if(Qt6_FOUND)
    # Link Qt dynamically (default behavior)
    target_link_libraries(koomesh_lib PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )

    # Ensure dynamic linking
    set_target_properties(koomesh_lib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()
```

## Building with Qt

### Prerequisites

Install Qt using one of these methods:

**Option 1: Official Qt Installer** (Recommended)
```bash
# Download from https://www.qt.io/download-open-source
# Install Qt 6.x with the following components:
# - Qt 6.5+ (Core, Widgets, OpenGL)
# - CMake support
```

**Option 2: Package Manager**

```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-declarative-dev libqt6opengl6-dev

# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel

# Arch Linux
sudo pacman -S qt6-base qt6-declarative

# macOS (Homebrew)
brew install qt@6

# Windows (vcpkg)
vcpkg install qt6-base qt6-declarative
```

### Build Configuration

```bash
mkdir build && cd build

# Let CMake find Qt automatically
cmake ..

# Or specify Qt installation path
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64 ..

# Build
cmake --build .
```

### Verifying Dynamic Linking

**Linux:**
```bash
ldd bin/koomesh | grep Qt
# Should show: libQt6Core.so => /path/to/libQt6Core.so.6
```

**macOS:**
```bash
otool -L bin/koomesh | grep Qt
# Should show: @rpath/QtCore.framework/Versions/6/QtCore
```

**Windows:**
```powershell
dumpbin /dependents koomesh.exe | findstr Qt
# Should show: Qt6Core.dll, Qt6Widgets.dll, etc.
```

## Distribution Guidelines

### Binary Distribution

When distributing KooMeshPrepost binaries:

1. **Include this document** (QT_LGPL_COMPLIANCE.md)
2. **Include Qt licenses**:
   - Copy `qt-licenses/` directory
   - Or provide link: https://www.qt.io/licensing
3. **Provide source code access**:
   - Include URL to source repository
   - Or include source code on request
4. **Ship Qt as shared libraries**:
   - Include Qt .dll/.so/.dylib files
   - Do NOT statically link Qt

**Example directory structure:**
```
koomesh/
├── bin/
│   ├── koomesh              # Main executable
│   └── lib/                 # Qt shared libraries
│       ├── libQt6Core.so.6
│       ├── libQt6Widgets.so.6
│       └── libQt6OpenGLWidgets.so.6
├── docs/
│   └── QT_LGPL_COMPLIANCE.md
├── qt-licenses/
│   ├── LGPL_EXCEPTION.txt
│   └── LICENSE.LGPL3
└── README.md
```

### Source Distribution

When distributing KooMeshPrepost source code:

1. **Include all source files**
2. **Include build instructions**
3. **Include this compliance document**
4. **No additional LGPL requirements** (source is freely available)

## Commercial Use

### ✓ LGPL Permits

- **Commercial use**: YES
- **Sell your application**: YES
- **Proprietary application**: YES
- **Closed-source application**: YES*

\* Your application can be closed-source, but:
- Must use Qt as shared library
- Must allow users to replace Qt libraries
- Must provide your object files OR source code for relinking
- Must provide this LGPL compliance documentation

### ✗ LGPL Prohibits

- **Static linking**: NO (violates LGPL unless you provide all object files)
- **Modifying Qt without sharing**: NO (must share Qt modifications under LGPL)
- **Removing LGPL notices**: NO (must preserve license information)

## FAQ

### Q: Can I sell software using Qt LGPL?
**A: YES**, as long as you follow LGPL requirements (dynamic linking, allow relinking, provide notices).

### Q: Can I keep my source code closed?
**A: YES**, your application code can be proprietary. Only Qt remains LGPL.

### Q: Do I need to provide my source code?
**A: NO**, but you must provide a way for users to relink with different Qt libraries.

### Q: Can I statically link Qt?
**A: NOT RECOMMENDED**. LGPL requires you to provide object files for relinking if you static link. Dynamic linking is much simpler.

### Q: What if I modify Qt source code?
**A: You must** release your Qt modifications under LGPL. Application code remains yours.

### Q: Do I need a commercial Qt license?
**A: NO**, if you comply with LGPL. Commercial license is optional and provides additional benefits (support, indemnification).

## Compliance Checklist

Before releasing KooMeshPrepost:

- [ ] Qt is dynamically linked (verify with `ldd`/`otool`/`dumpbin`)
- [ ] Qt license files included in distribution
- [ ] This compliance document included
- [ ] Source code repository URL provided
- [ ] No modifications to Qt source code
- [ ] Qt shared libraries included with binary distribution
- [ ] Build instructions provided

## References

- Qt LGPL License: https://www.qt.io/licensing/open-source-lgpl-obligations
- LGPL v3 Full Text: https://www.gnu.org/licenses/lgpl-3.0.html
- Qt Open Source: https://www.qt.io/download-open-source
- Qt Documentation: https://doc.qt.io/

## Contact

For questions about Qt licensing:
- Qt Company: https://www.qt.io/contact-us
- Free Software Foundation: https://www.fsf.org/

For questions about KooMeshPrepost's Qt usage:
- See project README or repository issues

---

**Last Updated**: 2025-11-10
**Qt Version**: 6.5+
**License**: LGPL v3
