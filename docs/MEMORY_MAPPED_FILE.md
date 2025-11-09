# Memory Mapped File

## Overview

The `MemoryMappedFile` class provides efficient file I/O through memory mapping, enabling direct memory access to file contents without explicit read operations. This is especially useful for large files where traditional I/O would be too slow.

## Features

- **Zero-copy access**: File data is mapped directly into process memory
- **Cross-platform**: Works on Linux/Unix (mmap) and Windows (CreateFileMapping)
- **Multiple access modes**: Read-only, read-write, copy-on-write
- **Partial mapping**: Map only a portion of the file
- **RAII-based**: Automatic resource cleanup
- **Performance hints**: Advise kernel about access patterns
- **Thread-safe**: Const methods are thread-safe

## Basic Usage

### Read-Only Access

```cpp
#include "io/MemoryMappedFile.h"

using namespace koomesh::io;

// Map entire file for reading
MemoryMappedFile mmf("large_mesh.k", MemoryMapMode::READ_ONLY);

if (mmf.isValid()) {
    const char* data = mmf.data();
    size_t size = mmf.size();

    // Process data directly
    for (size_t i = 0; i < size; ++i) {
        char c = data[i];
        // ...
    }
}
```

### Read-Write Access

```cpp
// Map file for reading and writing
MemoryMappedFile mmf("output.k", MemoryMapMode::READ_WRITE);

if (mmf.isValid()) {
    char* data = mmf.dataMutable();

    // Modify content directly
    std::memcpy(data, "HEADER", 6);

    // Flush changes to disk
    mmf.flush();
}
```

### Partial Mapping

```cpp
// Map only 1MB starting at offset 100MB
size_t offset = 100 * 1024 * 1024;
size_t size = 1 * 1024 * 1024;

MemoryMappedFile mmf("huge_file.dat", MemoryMapMode::READ_ONLY, offset, size);

// Process only the mapped region
const char* data = mmf.data();
```

### Copy-on-Write

```cpp
// Create private copy that doesn't affect original file
MemoryMappedFile mmf("template.k", MemoryMapMode::COPY_ON_WRITE);

char* data = mmf.dataMutable();
// Modifications are private to this process
std::memcpy(data, "MODIFIED", 8);
// Original file remains unchanged
```

### Scoped Mapping

```cpp
#include "io/MemoryMappedFile.h"

{
    ScopedMemoryMap scoped("file.dat", MemoryMapMode::READ_ONLY);

    if (scoped.isValid()) {
        // Use the mapping
        const char* data = scoped.data();
        size_t size = scoped.size();
    }

    // Automatically unmapped when scope ends
}
```

## Access Modes

### READ_ONLY
- File is opened for reading only
- `dataMutable()` returns nullptr
- Most efficient for reading
- Multiple processes can map the same file

### READ_WRITE
- File is opened for reading and writing
- Changes are visible to other processes
- `flush()` writes changes to disk
- Use for modifying files

### COPY_ON_WRITE
- File is opened read-only, but writes are allowed
- Changes are private to the process (not written to file)
- Useful for temporary modifications
- Also known as "private mapping"

## Performance Optimization

### Access Pattern Hints

```cpp
MemoryMappedFile mmf("sequential.dat", MemoryMapMode::READ_ONLY);

// Tell kernel we'll read sequentially
mmf.advise(MemoryMappedFile::Advice::SEQUENTIAL);

// Or for random access
mmf.advise(MemoryMappedFile::Advice::RANDOM);

// Prefetch data
mmf.advise(MemoryMappedFile::Advice::WILLNEED);
```

### Advice Types

| Advice | Description | Use Case |
|--------|-------------|----------|
| `NORMAL` | Default behavior | No specific access pattern |
| `SEQUENTIAL` | Read data sequentially | Parsing files line by line |
| `RANDOM` | Random access pattern | Hash table backed by file |
| `WILLNEED` | Data will be needed soon | Prefetch for faster access |
| `DONTNEED` | Data won't be needed | Free memory for other uses |

## Page Alignment

Memory mapping requires page-aligned offsets:

```cpp
// Get system page size (typically 4096 bytes)
size_t pageSize = MemoryMappedFile::getPageSize();

// Align offset to page boundary
size_t offset = 12345;
size_t aligned = MemoryMappedFile::alignToPage(offset);  // 8192 (if pageSize=4096)

MemoryMappedFile mmf("file.dat", MemoryMapMode::READ_ONLY, aligned);
```

## Error Handling

```cpp
#include "io/FileIOException.h"

try {
    MemoryMappedFile mmf("file.dat", MemoryMapMode::READ_ONLY);

    if (!mmf.isValid()) {
        // Mapping failed
        std::cerr << "Failed to map file\n";
        return;
    }

    // Use mmf...

} catch (const FileOpenException& e) {
    std::cerr << "Cannot open file: " << e.what() << "\n";
} catch (const FileIOException& e) {
    std::cerr << "I/O error: " << e.what() << "\n";
}
```

## Move Semantics

```cpp
MemoryMappedFile createMapping(const std::string& filename) {
    MemoryMappedFile mmf(filename, MemoryMapMode::READ_ONLY);
    return mmf;  // Move, not copy
}

MemoryMappedFile mmf1 = createMapping("file.dat");

// Transfer ownership
MemoryMappedFile mmf2 = std::move(mmf1);

// mmf1 is now invalid, mmf2 owns the mapping
```

## Platform-Specific Details

### Linux/Unix
- Uses `mmap()` and `munmap()`
- Supports `madvise()` for performance hints
- Page size typically 4096 bytes
- Maximum mapping size: system-dependent

### Windows
- Uses `CreateFileMapping()` and `MapViewOfFile()`
- `madvise()` equivalents not available
- Page size: 64KB granularity
- Maximum mapping size: 2^47 bytes (64-bit)

## Performance Considerations

### Advantages
- **Zero-copy**: No data copying between kernel and user space
- **Lazy loading**: OS loads pages on-demand
- **Shared memory**: Multiple processes share same physical pages
- **Large files**: Can map files larger than available RAM

### Disadvantages
- **Page faults**: First access to each page causes page fault
- **Memory pressure**: Large mappings can cause swapping
- **Address space**: Limited by virtual address space (32-bit)
- **Alignment**: Requires page-aligned offsets

### Best Practices

1. **Use for large files** (> 100KB)
   - Small files: traditional I/O may be faster
   - Large files: memory mapping excels

2. **Advise access pattern**
   ```cpp
   mmf.advise(MemoryMappedFile::Advice::SEQUENTIAL);
   ```

3. **Unmap when done**
   ```cpp
   mmf.close();  // Free resources immediately
   ```

4. **Align offsets to pages**
   ```cpp
   size_t aligned = MemoryMappedFile::alignToPage(offset);
   ```

5. **Flush write mappings**
   ```cpp
   mmf.flush();  // Ensure changes are written
   ```

## Example: LS-DYNA File Reader

```cpp
class LSDynaReader {
public:
    ReadResult read(const std::string& filename, Mesh& mesh) {
        // Map file for reading
        MemoryMappedFile mmf(filename, MemoryMapMode::READ_ONLY);

        if (!mmf.isValid()) {
            throw FileOpenException(filename, "Cannot map file");
        }

        // Advise sequential reading
        mmf.advise(MemoryMappedFile::Advice::SEQUENTIAL);

        const char* data = mmf.data();
        size_t size = mmf.size();

        // Parse file line by line
        const char* end = data + size;
        const char* line = data;

        while (line < end) {
            const char* lineEnd = std::find(line, end, '\n');

            // Process line
            parseLine(line, lineEnd - line, mesh);

            line = lineEnd + 1;
        }

        return ReadResult{/* ... */};
    }
};
```

## Testing

The class includes comprehensive tests covering:

- Basic open/close operations
- Read-only, read-write, and copy-on-write modes
- Partial mapping with offsets
- Move semantics
- Large file handling (10+ MB)
- Error conditions
- Multiple simultaneous mappings
- Scoped mapping RAII

Run tests:
```bash
cd build
ctest -R MemoryMappedFileTests -V
```

## Thread Safety

- **Const methods**: Thread-safe (multiple threads can read simultaneously)
- **Mutable methods**: Not thread-safe (requires external synchronization)
- **Multiple mappings**: Each thread should have its own `MemoryMappedFile` instance

```cpp
// Thread-safe: multiple readers
void readerThread(const MemoryMappedFile& mmf) {
    const char* data = mmf.data();  // OK
    // Read data...
}

// Not thread-safe: concurrent writers need mutex
std::mutex mtx;

void writerThread(MemoryMappedFile& mmf) {
    std::lock_guard<std::mutex> lock(mtx);
    char* data = mmf.dataMutable();  // Protected
    // Write data...
}
```

## Limitations

1. **File size**: Cannot exceed available virtual address space
2. **Offset alignment**: Must be page-aligned
3. **Atomicity**: Write operations are not atomic
4. **Portability**: Behavior may differ between platforms
5. **Empty files**: Cannot map zero-length files

## See Also

- [File I/O Interface Design](FILE_IO_INTERFACES.md)
- [LS-DYNA Keyword Format](LSDYNA_KEYWORD_FORMAT.md)
- POSIX `mmap(2)` man page
- Windows `CreateFileMapping` documentation
