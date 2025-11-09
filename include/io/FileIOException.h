#pragma once

#include <exception>
#include <string>

namespace koomesh {
namespace io {

/**
 * @brief Base exception class for file I/O operations
 */
class FileIOException : public std::exception {
public:
    explicit FileIOException(const std::string& message)
        : m_message(message) {}

    const char* what() const noexcept override {
        return m_message.c_str();
    }

    const std::string& message() const noexcept {
        return m_message;
    }

protected:
    std::string m_message;
};

/**
 * @brief Exception thrown when file cannot be opened
 */
class FileOpenException : public FileIOException {
public:
    FileOpenException(const std::string& filename, const std::string& reason)
        : FileIOException("Failed to open file '" + filename + "': " + reason),
          m_filename(filename) {}

    const std::string& filename() const noexcept {
        return m_filename;
    }

private:
    std::string m_filename;
};

/**
 * @brief Exception thrown when file reading fails
 */
class FileReadException : public FileIOException {
public:
    FileReadException(const std::string& filename, size_t lineNumber, const std::string& reason)
        : FileIOException("Read error in file '" + filename + "' at line " +
                         std::to_string(lineNumber) + ": " + reason),
          m_filename(filename),
          m_lineNumber(lineNumber) {}

    const std::string& filename() const noexcept {
        return m_filename;
    }

    size_t lineNumber() const noexcept {
        return m_lineNumber;
    }

private:
    std::string m_filename;
    size_t m_lineNumber;
};

/**
 * @brief Exception thrown when file writing fails
 */
class FileWriteException : public FileIOException {
public:
    FileWriteException(const std::string& filename, const std::string& reason)
        : FileIOException("Write error in file '" + filename + "': " + reason),
          m_filename(filename) {}

    const std::string& filename() const noexcept {
        return m_filename;
    }

private:
    std::string m_filename;
};

/**
 * @brief Exception thrown when file format is invalid
 */
class InvalidFormatException : public FileIOException {
public:
    InvalidFormatException(const std::string& filename, size_t lineNumber,
                          const std::string& expected, const std::string& actual)
        : FileIOException("Invalid format in file '" + filename + "' at line " +
                         std::to_string(lineNumber) + ": expected " + expected +
                         ", got " + actual),
          m_filename(filename),
          m_lineNumber(lineNumber),
          m_expected(expected),
          m_actual(actual) {}

    const std::string& filename() const noexcept {
        return m_filename;
    }

    size_t lineNumber() const noexcept {
        return m_lineNumber;
    }

    const std::string& expected() const noexcept {
        return m_expected;
    }

    const std::string& actual() const noexcept {
        return m_actual;
    }

private:
    std::string m_filename;
    size_t m_lineNumber;
    std::string m_expected;
    std::string m_actual;
};

/**
 * @brief Exception thrown when operation is cancelled
 */
class OperationCancelledException : public FileIOException {
public:
    OperationCancelledException()
        : FileIOException("Operation was cancelled by user") {}
};

/**
 * @brief Exception thrown when file format is not supported
 */
class UnsupportedFormatException : public FileIOException {
public:
    explicit UnsupportedFormatException(const std::string& format)
        : FileIOException("Unsupported file format: " + format),
          m_format(format) {}

    const std::string& format() const noexcept {
        return m_format;
    }

private:
    std::string m_format;
};

} // namespace io
} // namespace koomesh
