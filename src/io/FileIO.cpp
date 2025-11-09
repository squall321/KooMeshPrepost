#include "io/IFileReader.h"
#include "io/IFileWriter.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace koomesh {
namespace io {

// ======================================================================
// ReadResult Implementation
// ======================================================================

std::string ReadResult::generateReport() const {
    std::ostringstream oss;

    oss << "=================================================\n";
    oss << "           FILE READ REPORT\n";
    oss << "=================================================\n\n";

    // Status
    oss << "STATUS: " << (success ? "SUCCESS" : "FAILED") << "\n";
    if (!message.empty()) {
        oss << "Message: " << message << "\n";
    }
    oss << "\n";

    // Statistics
    oss << "DATA READ:\n";
    oss << "  Nodes:    " << std::setw(10) << nodesRead << "\n";
    oss << "  Elements: " << std::setw(10) << elementsRead << "\n";
    oss << "  Parts:    " << std::setw(10) << partsRead << "\n";
    oss << "\n";

    if (nodesSkipped > 0 || elementsSkipped > 0) {
        oss << "SKIPPED:\n";
        if (nodesSkipped > 0) {
            oss << "  Nodes:    " << std::setw(10) << nodesSkipped << "\n";
        }
        if (elementsSkipped > 0) {
            oss << "  Elements: " << std::setw(10) << elementsSkipped << "\n";
        }
        oss << "\n";
    }

    // Issues
    if (errorCount > 0 || warningCount > 0) {
        oss << "ISSUES:\n";
        oss << "  Errors:   " << std::setw(10) << errorCount << "\n";
        oss << "  Warnings: " << std::setw(10) << warningCount << "\n";
        oss << "\n";

        if (!errors.empty() && errors.size() <= 10) {
            oss << "ERROR DETAILS:\n";
            for (const auto& error : errors) {
                oss << "  - " << error << "\n";
            }
            oss << "\n";
        } else if (errors.size() > 10) {
            oss << "ERROR DETAILS (first 10):\n";
            for (size_t i = 0; i < 10; ++i) {
                oss << "  - " << errors[i] << "\n";
            }
            oss << "  ... (" << (errors.size() - 10) << " more)\n\n";
        }

        if (!warnings.empty() && warnings.size() <= 10) {
            oss << "WARNING DETAILS:\n";
            for (const auto& warning : warnings) {
                oss << "  - " << warning << "\n";
            }
            oss << "\n";
        } else if (warnings.size() > 10) {
            oss << "WARNING DETAILS (first 10):\n";
            for (size_t i = 0; i < 10; ++i) {
                oss << "  - " << warnings[i] << "\n";
            }
            oss << "  ... (" << (warnings.size() - 10) << " more)\n\n";
        }
    }

    // Performance
    oss << "PERFORMANCE:\n";
    oss << "  Read Time: " << std::fixed << std::setprecision(3)
        << readTimeSeconds << " seconds\n";

    if (readTimeSeconds > 0.0) {
        size_t totalItems = nodesRead + elementsRead;
        double itemsPerSecond = totalItems / readTimeSeconds;
        oss << "  Throughput: " << std::fixed << std::setprecision(0)
            << itemsPerSecond << " items/sec\n";
    }

    oss << "\n=================================================\n";

    return oss.str();
}

// ======================================================================
// WriteResult Implementation
// ======================================================================

std::string WriteResult::generateReport() const {
    std::ostringstream oss;

    oss << "=================================================\n";
    oss << "           FILE WRITE REPORT\n";
    oss << "=================================================\n\n";

    // Status
    oss << "STATUS: " << (success ? "SUCCESS" : "FAILED") << "\n";
    if (!message.empty()) {
        oss << "Message: " << message << "\n";
    }
    oss << "\n";

    // Statistics
    oss << "DATA WRITTEN:\n";
    oss << "  Nodes:    " << std::setw(10) << nodesWritten << "\n";
    oss << "  Elements: " << std::setw(10) << elementsWritten << "\n";
    oss << "  Parts:    " << std::setw(10) << partsWritten << "\n";
    oss << "\n";

    // File size
    oss << "FILE SIZE:\n";
    if (bytesWritten < 1024) {
        oss << "  " << bytesWritten << " bytes\n";
    } else if (bytesWritten < 1024 * 1024) {
        oss << "  " << std::fixed << std::setprecision(2)
            << (bytesWritten / 1024.0) << " KB\n";
    } else if (bytesWritten < 1024 * 1024 * 1024) {
        oss << "  " << std::fixed << std::setprecision(2)
            << (bytesWritten / (1024.0 * 1024.0)) << " MB\n";
    } else {
        oss << "  " << std::fixed << std::setprecision(2)
            << (bytesWritten / (1024.0 * 1024.0 * 1024.0)) << " GB\n";
    }
    oss << "\n";

    // Issues
    if (errorCount > 0 || warningCount > 0) {
        oss << "ISSUES:\n";
        oss << "  Errors:   " << std::setw(10) << errorCount << "\n";
        oss << "  Warnings: " << std::setw(10) << warningCount << "\n";
        oss << "\n";

        if (!errors.empty() && errors.size() <= 10) {
            oss << "ERROR DETAILS:\n";
            for (const auto& error : errors) {
                oss << "  - " << error << "\n";
            }
            oss << "\n";
        }

        if (!warnings.empty() && warnings.size() <= 10) {
            oss << "WARNING DETAILS:\n";
            for (const auto& warning : warnings) {
                oss << "  - " << warning << "\n";
            }
            oss << "\n";
        }
    }

    // Performance
    oss << "PERFORMANCE:\n";
    oss << "  Write Time: " << std::fixed << std::setprecision(3)
        << writeTimeSeconds << " seconds\n";

    if (writeTimeSeconds > 0.0) {
        double mbPerSecond = (bytesWritten / (1024.0 * 1024.0)) / writeTimeSeconds;
        oss << "  Throughput: " << std::fixed << std::setprecision(2)
            << mbPerSecond << " MB/sec\n";
    }

    oss << "\n=================================================\n";

    return oss.str();
}

// ======================================================================
// FileWriterFactory Implementation
// ======================================================================

std::vector<FileWriterFactory::WriterCreator>& FileWriterFactory::getCreators() {
    static std::vector<WriterCreator> creators;
    return creators;
}

void FileWriterFactory::registerWriter(WriterCreator creator) {
    getCreators().push_back(creator);
}

std::unique_ptr<IFileWriter> FileWriterFactory::createWriter(const std::string& filename) {
    for (const auto& creator : getCreators()) {
        auto writer = creator();
        if (writer && writer->canWrite(filename)) {
            return writer;
        }
    }
    return nullptr;
}

std::vector<std::unique_ptr<IFileWriter>> FileWriterFactory::getAllWriters() {
    std::vector<std::unique_ptr<IFileWriter>> writers;
    for (const auto& creator : getCreators()) {
        writers.push_back(creator());
    }
    return writers;
}

} // namespace io
} // namespace koomesh
