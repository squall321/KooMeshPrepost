#include "io/MemoryMappedFile.h"
#include "io/FileIOException.h"

#include <stdexcept>
#include <cstring>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace koomesh {
namespace io {

// ======================================================================
// Platform-specific helpers
// ======================================================================

#ifndef _WIN32
// Get file size using fstat
static size_t getFileSize(int fd) {
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        return 0;
    }
    return static_cast<size_t>(sb.st_size);
}
#endif

// ======================================================================
// MemoryMappedFile Implementation
// ======================================================================

MemoryMappedFile::MemoryMappedFile() = default;

MemoryMappedFile::MemoryMappedFile(
    const std::string& filename,
    MemoryMapMode mode,
    size_t offset,
    size_t size)
    : m_filename(filename),
      m_offset(offset),
      m_mode(mode)
{
    if (!open(filename, mode, offset, size)) {
        throw FileOpenException(filename, "Failed to memory map file");
    }
}

MemoryMappedFile::~MemoryMappedFile() {
    close();
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : m_filename(std::move(other.m_filename)),
      m_data(other.m_data),
      m_size(other.m_size),
      m_fileSize(other.m_fileSize),
      m_offset(other.m_offset),
      m_mode(other.m_mode)
#ifdef _WIN32
    , m_fileHandle(other.m_fileHandle),
      m_mappingHandle(other.m_mappingHandle)
#else
    , m_fileDescriptor(other.m_fileDescriptor),
      m_mappedBase(other.m_mappedBase),
      m_mappedSize(other.m_mappedSize)
#endif
{
    other.m_data = nullptr;
    other.m_size = 0;
    other.m_fileSize = 0;
    other.m_offset = 0;
#ifdef _WIN32
    other.m_fileHandle = nullptr;
    other.m_mappingHandle = nullptr;
#else
    other.m_fileDescriptor = -1;
    other.m_mappedBase = nullptr;
    other.m_mappedSize = 0;
#endif
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this != &other) {
        close();

        m_filename = std::move(other.m_filename);
        m_data = other.m_data;
        m_size = other.m_size;
        m_fileSize = other.m_fileSize;
        m_offset = other.m_offset;
        m_mode = other.m_mode;

#ifdef _WIN32
        m_fileHandle = other.m_fileHandle;
        m_mappingHandle = other.m_mappingHandle;
        other.m_fileHandle = nullptr;
        other.m_mappingHandle = nullptr;
#else
        m_fileDescriptor = other.m_fileDescriptor;
        m_mappedBase = other.m_mappedBase;
        m_mappedSize = other.m_mappedSize;
        other.m_fileDescriptor = -1;
        other.m_mappedBase = nullptr;
        other.m_mappedSize = 0;
#endif

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_fileSize = 0;
        other.m_offset = 0;
    }
    return *this;
}

bool MemoryMappedFile::open(
    const std::string& filename,
    MemoryMapMode mode,
    size_t offset,
    size_t size)
{
    close();

    m_filename = filename;
    m_mode = mode;
    m_offset = offset;
    m_size = size;  // Store the requested size

    return openImpl();
}

void MemoryMappedFile::close() {
    closeImpl();
}

char* MemoryMappedFile::dataMutable() {
    if (m_mode == MemoryMapMode::READ_ONLY) {
        return nullptr;
    }
    return m_data;
}

bool MemoryMappedFile::flush(bool async) {
    if (!isValid() || m_mode == MemoryMapMode::READ_ONLY) {
        return false;
    }

#ifdef _WIN32
    return FlushViewOfFile(m_data, m_size) != 0;
#else
    int flags = async ? MS_ASYNC : MS_SYNC;
    return msync(m_data, m_size, flags) == 0;
#endif
}

bool MemoryMappedFile::advise(Advice advice) {
    if (!isValid()) {
        return false;
    }

#ifdef _WIN32
    // Windows doesn't have direct madvise equivalent
    // Could use PrefetchVirtualMemory on Windows 8+
    (void)advice;
    return false;
#else
    int madv;
    switch (advice) {
        case Advice::NORMAL:     madv = MADV_NORMAL; break;
        case Advice::SEQUENTIAL: madv = MADV_SEQUENTIAL; break;
        case Advice::RANDOM:     madv = MADV_RANDOM; break;
        case Advice::WILLNEED:   madv = MADV_WILLNEED; break;
        case Advice::DONTNEED:   madv = MADV_DONTNEED; break;
        default:                 madv = MADV_NORMAL; break;
    }
    return madvise(m_data, m_size, madv) == 0;
#endif
}

size_t MemoryMappedFile::getPageSize() {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return static_cast<size_t>(sysInfo.dwAllocationGranularity);
#else
    return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
}

size_t MemoryMappedFile::alignToPage(size_t offset) {
    size_t pageSize = getPageSize();
    return (offset / pageSize) * pageSize;
}

// ======================================================================
// Platform-specific implementations
// ======================================================================

#ifdef _WIN32

bool MemoryMappedFile::openImpl() {
    // Determine access flags
    DWORD desiredAccess = 0;
    DWORD shareMode = FILE_SHARE_READ;
    DWORD creationDisposition = OPEN_EXISTING;
    DWORD pageProtection = 0;
    DWORD mapAccess = 0;

    switch (m_mode) {
        case MemoryMapMode::READ_ONLY:
            desiredAccess = GENERIC_READ;
            pageProtection = PAGE_READONLY;
            mapAccess = FILE_MAP_READ;
            break;

        case MemoryMapMode::READ_WRITE:
            desiredAccess = GENERIC_READ | GENERIC_WRITE;
            shareMode = 0;
            pageProtection = PAGE_READWRITE;
            mapAccess = FILE_MAP_WRITE;
            break;

        case MemoryMapMode::COPY_ON_WRITE:
            desiredAccess = GENERIC_READ;
            pageProtection = PAGE_WRITECOPY;
            mapAccess = FILE_MAP_COPY;
            break;
    }

    // Open file
    m_fileHandle = CreateFileA(
        m_filename.c_str(),
        desiredAccess,
        shareMode,
        nullptr,
        creationDisposition,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (m_fileHandle == INVALID_HANDLE_VALUE) {
        closeImpl();
        return false;
    }

    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(m_fileHandle, &fileSize)) {
        closeImpl();
        return false;
    }
    m_fileSize = static_cast<size_t>(fileSize.QuadPart);

    // Check if offset is beyond file size
    if (m_offset >= m_fileSize) {
        closeImpl();
        return false;
    }

    // Determine mapping size
    if (m_size == 0) {
        m_size = m_fileSize - m_offset;
    }

    if (m_offset + m_size > m_fileSize) {
        closeImpl();
        return false;
    }

    // Create file mapping
    DWORD sizeHigh = static_cast<DWORD>((m_fileSize >> 32) & 0xFFFFFFFF);
    DWORD sizeLow = static_cast<DWORD>(m_fileSize & 0xFFFFFFFF);

    m_mappingHandle = CreateFileMappingA(
        m_fileHandle,
        nullptr,
        pageProtection,
        sizeHigh,
        sizeLow,
        nullptr);

    if (m_mappingHandle == nullptr) {
        closeImpl();
        return false;
    }

    // Map view of file
    DWORD offsetHigh = static_cast<DWORD>((m_offset >> 32) & 0xFFFFFFFF);
    DWORD offsetLow = static_cast<DWORD>(m_offset & 0xFFFFFFFF);

    void* mapped = MapViewOfFile(
        m_mappingHandle,
        mapAccess,
        offsetHigh,
        offsetLow,
        m_size);

    if (mapped == nullptr) {
        closeImpl();
        return false;
    }

    m_data = static_cast<char*>(mapped);
    return true;
}

void MemoryMappedFile::closeImpl() {
    if (m_data != nullptr) {
        UnmapViewOfFile(m_data);
        m_data = nullptr;
    }

    if (m_mappingHandle != nullptr) {
        CloseHandle(m_mappingHandle);
        m_mappingHandle = nullptr;
    }

    if (m_fileHandle != nullptr && m_fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_fileHandle);
        m_fileHandle = nullptr;
    }

    m_size = 0;
    m_fileSize = 0;
}

#else // Linux/Unix

bool MemoryMappedFile::openImpl() {
    // Determine open flags
    int openFlags = 0;
    int protection = 0;
    int mapFlags = MAP_SHARED;

    switch (m_mode) {
        case MemoryMapMode::READ_ONLY:
            openFlags = O_RDONLY;
            protection = PROT_READ;
            break;

        case MemoryMapMode::READ_WRITE:
            openFlags = O_RDWR;
            protection = PROT_READ | PROT_WRITE;
            break;

        case MemoryMapMode::COPY_ON_WRITE:
            openFlags = O_RDONLY;
            protection = PROT_READ | PROT_WRITE;
            mapFlags = MAP_PRIVATE;
            break;
    }

    // Open file
    m_fileDescriptor = ::open(m_filename.c_str(), openFlags);
    if (m_fileDescriptor == -1) {
        closeImpl();
        return false;
    }

    // Get file size
    m_fileSize = getFileSize(m_fileDescriptor);
    if (m_fileSize == 0) {
        closeImpl();
        return false;
    }

    // Store user-requested offset and size
    size_t userOffset = m_offset;
    size_t userSize = m_size;

    // Check if offset is beyond file size
    if (userOffset >= m_fileSize) {
        closeImpl();
        return false;
    }

    // Determine mapping size
    if (userSize == 0) {
        userSize = m_fileSize - userOffset;
    }

    if (userOffset + userSize > m_fileSize) {
        closeImpl();
        return false;
    }

    // Align offset to page boundary for mmap
    // mmap requires offset to be page-aligned
    size_t alignedOffset = alignToPage(userOffset);
    size_t offsetDelta = userOffset - alignedOffset;
    size_t alignedSize = userSize + offsetDelta;

    // Map file with aligned offset
    void* mapped = mmap(
        nullptr,
        alignedSize,
        protection,
        mapFlags,
        m_fileDescriptor,
        static_cast<off_t>(alignedOffset));

    if (mapped == MAP_FAILED) {
        closeImpl();
        return false;
    }

    // Store the actual mmap base and size for munmap
    m_mappedBase = static_cast<char*>(mapped);
    m_mappedSize = alignedSize;

    // Adjust data pointer to user-requested offset within the mapping
    m_data = m_mappedBase + offsetDelta;
    m_size = userSize;

    // m_offset should remain as the user-requested offset for offset() accessor
    // m_offset is already set to userOffset

    return true;
}

void MemoryMappedFile::closeImpl() {
    if (m_mappedBase != nullptr) {
        munmap(m_mappedBase, m_mappedSize);
        m_mappedBase = nullptr;
        m_mappedSize = 0;
    }

    if (m_fileDescriptor != -1) {
        ::close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }

    m_data = nullptr;
    m_size = 0;
    m_fileSize = 0;
}

#endif

} // namespace io
} // namespace koomesh
