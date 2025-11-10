#pragma once

#include <cstddef>
#include <string>
#include <memory>

namespace koomesh {
namespace io {

/**
 * @brief Access mode for memory mapped files
 */
enum class MemoryMapMode {
    READ_ONLY,      ///< Read-only access
    READ_WRITE,     ///< Read-write access
    COPY_ON_WRITE   ///< Copy-on-write (private mapping)
};

/**
 * @brief Memory mapped file for efficient large file I/O
 *
 * Platform-specific implementation using:
 * - Linux/Unix: mmap/munmap
 * - Windows: CreateFileMapping/MapViewOfFile
 *
 * Features:
 * - RAII-based automatic cleanup
 * - Zero-copy file access
 * - Support for partial mapping (offset + size)
 * - Thread-safe (const methods)
 *
 * Example:
 * @code
 * MemoryMappedFile mmf("large_file.k", MemoryMapMode::READ_ONLY);
 * if (mmf.isValid()) {
 *     const char* data = mmf.data();
 *     size_t size = mmf.size();
 *     // Process data directly without copying
 * }
 * @endcode
 */
class MemoryMappedFile {
public:
    /**
     * @brief Construct without opening a file
     */
    MemoryMappedFile();

    /**
     * @brief Construct and open a file
     *
     * @param filename Path to file
     * @param mode Access mode
     * @param offset Offset in bytes from start of file (must be page-aligned)
     * @param size Number of bytes to map (0 = entire file)
     *
     * @throws FileOpenException if file cannot be opened
     * @throws FileIOException if mapping fails
     */
    explicit MemoryMappedFile(
        const std::string& filename,
        MemoryMapMode mode = MemoryMapMode::READ_ONLY,
        size_t offset = 0,
        size_t size = 0);

    /**
     * @brief Destructor - automatically unmaps the file
     */
    ~MemoryMappedFile();

    // Non-copyable
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    // Movable
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;

    /**
     * @brief Open and map a file
     *
     * @param filename Path to file
     * @param mode Access mode
     * @param offset Offset in bytes (must be page-aligned)
     * @param size Number of bytes to map (0 = entire file)
     * @return true if successful
     */
    bool open(
        const std::string& filename,
        MemoryMapMode mode = MemoryMapMode::READ_ONLY,
        size_t offset = 0,
        size_t size = 0);

    /**
     * @brief Close and unmap the file
     */
    void close();

    /**
     * @brief Check if file is currently mapped
     */
    bool isValid() const { return m_data != nullptr; }

    /**
     * @brief Get pointer to mapped data
     *
     * @return Pointer to data, or nullptr if not mapped
     */
    const char* data() const { return m_data; }

    /**
     * @brief Get mutable pointer to mapped data
     *
     * Only available for READ_WRITE and COPY_ON_WRITE modes
     *
     * @return Mutable pointer to data, or nullptr if not mapped
     */
    char* dataMutable();

    /**
     * @brief Get size of mapped region in bytes
     */
    size_t size() const { return m_size; }

    /**
     * @brief Get original file size in bytes
     */
    size_t fileSize() const { return m_fileSize; }

    /**
     * @brief Get mapping offset from start of file
     */
    size_t offset() const { return m_offset; }

    /**
     * @brief Get access mode
     */
    MemoryMapMode mode() const { return m_mode; }

    /**
     * @brief Get filename
     */
    const std::string& filename() const { return m_filename; }

    /**
     * @brief Flush changes to disk (for writable mappings)
     *
     * @param async If true, request asynchronous flush
     * @return true if successful
     */
    bool flush(bool async = false);

    /**
     * @brief Advise kernel about expected access pattern
     *
     * Platform-specific optimization hints:
     * - NORMAL: No special treatment
     * - SEQUENTIAL: Will be accessed sequentially
     * - RANDOM: Will be accessed randomly
     * - WILLNEED: Will be needed soon (prefetch)
     * - DONTNEED: Won't be needed soon (can free)
     */
    enum class Advice {
        NORMAL,
        SEQUENTIAL,
        RANDOM,
        WILLNEED,
        DONTNEED
    };

    /**
     * @brief Provide access pattern hint to kernel
     *
     * @param advice Access pattern hint
     * @return true if successful (may not be supported on all platforms)
     */
    bool advise(Advice advice);

    /**
     * @brief Get system page size
     */
    static size_t getPageSize();

    /**
     * @brief Align offset to page boundary
     *
     * @param offset Original offset
     * @return Aligned offset (rounded down to page boundary)
     */
    static size_t alignToPage(size_t offset);

private:
    std::string m_filename;
    char* m_data = nullptr;
    size_t m_size = 0;
    size_t m_fileSize = 0;
    size_t m_offset = 0;
    MemoryMapMode m_mode = MemoryMapMode::READ_ONLY;

    // Platform-specific handles
#ifdef _WIN32
    void* m_fileHandle = nullptr;      // HANDLE
    void* m_mappingHandle = nullptr;   // HANDLE
#else
    int m_fileDescriptor = -1;
    char* m_mappedBase = nullptr;      // Actual mmap base (for munmap)
    size_t m_mappedSize = 0;           // Actual mmap size (for munmap)
#endif

    // Helper methods
    bool openImpl();
    void closeImpl();
};

/**
 * @brief RAII wrapper for temporary memory-mapped file access
 *
 * Useful for scope-based automatic unmapping
 */
class ScopedMemoryMap {
public:
    explicit ScopedMemoryMap(
        const std::string& filename,
        MemoryMapMode mode = MemoryMapMode::READ_ONLY)
        : m_mmf(filename, mode) {}

    ~ScopedMemoryMap() = default;

    const char* data() const { return m_mmf.data(); }
    char* dataMutable() { return m_mmf.dataMutable(); }
    size_t size() const { return m_mmf.size(); }
    bool isValid() const { return m_mmf.isValid(); }

    MemoryMappedFile& get() { return m_mmf; }
    const MemoryMappedFile& get() const { return m_mmf; }

private:
    MemoryMappedFile m_mmf;
};

} // namespace io
} // namespace koomesh
