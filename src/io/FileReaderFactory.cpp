#include "io/FileReaderFactory.h"
#include "io/LSDynaFileReader.h"
#include "io/STLFileReader.h"
#include "io/VTKFileReader.h"
#include "io/NastranFileReader.h"
#include "io/FileIOException.h"
#include <stdexcept>

namespace koomesh {
namespace io {

// Static member initialization
bool FileReaderFactory::s_initialized = false;

// ======================================================================
// Public Methods
// ======================================================================

std::unique_ptr<IFileReader> FileReaderFactory::create(FileFormat format) {
    // Ensure default readers are registered
    if (!s_initialized) {
        initializeDefaultReaders();
    }

    auto& registry = getRegistry();
    auto it = registry.find(format);

    if (it == registry.end()) {
        return nullptr;
    }

    return it->second();
}

std::unique_ptr<IFileReader> FileReaderFactory::createFromFile(const std::string& filepath) {
    // Detect file format
    FileFormat format = FileFormatDetector::detect(filepath);

    if (format == FileFormat::UNKNOWN) {
        throw FileIOException("Cannot detect file format: " + filepath);
    }

    // Create reader
    auto reader = create(format);

    if (!reader) {
        throw FileIOException(
            "No reader available for format: " +
            FileFormatDetector::formatName(format));
    }

    return reader;
}

ReadResult FileReaderFactory::readFile(
    const std::string& filepath,
    core::Mesh& mesh,
    const ReadOptions& options,
    ProgressCallback progressCallback)
{
    // Create reader based on file
    auto reader = createFromFile(filepath);

    if (!reader) {
        ReadResult result;
        result.success = false;
        result.message = "Failed to create reader for file: " + filepath;
        return result;
    }

    // Read file
    return reader->read(filepath, mesh, options, progressCallback);
}

void FileReaderFactory::registerReader(FileFormat format, ReaderCreator creator) {
    if (!creator) {
        throw std::invalid_argument("Reader creator function cannot be null");
    }

    auto& registry = getRegistry();
    registry[format] = std::move(creator);
}

bool FileReaderFactory::isFormatSupported(FileFormat format) {
    if (!s_initialized) {
        initializeDefaultReaders();
    }

    auto& registry = getRegistry();
    return registry.find(format) != registry.end();
}

std::vector<FileFormat> FileReaderFactory::getSupportedFormats() {
    if (!s_initialized) {
        initializeDefaultReaders();
    }

    auto& registry = getRegistry();
    std::vector<FileFormat> formats;
    formats.reserve(registry.size());

    for (const auto& [format, creator] : registry) {
        formats.push_back(format);
    }

    return formats;
}

// ======================================================================
// Private Methods
// ======================================================================

std::map<FileFormat, FileReaderFactory::ReaderCreator>&
FileReaderFactory::getRegistry() {
    // Singleton pattern: static local variable
    static std::map<FileFormat, ReaderCreator> registry;
    return registry;
}

void FileReaderFactory::initializeDefaultReaders() {
    if (s_initialized) {
        return;
    }

    auto& registry = getRegistry();

    // Register LS-DYNA reader
    registry[FileFormat::LSDYNA] = []() {
        return std::make_unique<LSDynaFileReader>();
    };

    // Register STL reader
    registry[FileFormat::STL] = []() {
        return std::make_unique<STLFileReader>();
    };

    // Register VTK reader (stub for now)
    registry[FileFormat::VTK] = []() {
        return std::make_unique<VTKFileReader>();
    };

    // Register Nastran reader (stub for now)
    registry[FileFormat::NASTRAN] = []() {
        return std::make_unique<NastranFileReader>();
    };

    s_initialized = true;
}

} // namespace io
} // namespace koomesh
