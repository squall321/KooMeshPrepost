#include "core/PerformanceDocGenerator.h"
#include "core/SpatialIndexFactory.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <chrono>
#include <ctime>

namespace koomesh {
namespace core {

PerformanceDocGenerator::PerformanceDocGenerator()
    : m_os("Unknown OS")
    , m_cpu("Unknown CPU")
    , m_memory("Unknown Memory")
    , m_projectName("KooMeshPrepost")
    , m_version("1.0.0")
{
}

void PerformanceDocGenerator::setSystemInfo(const std::string& os,
                                            const std::string& cpu,
                                            const std::string& memory) {
    m_os = os;
    m_cpu = cpu;
    m_memory = memory;
}

void PerformanceDocGenerator::setProjectInfo(const std::string& projectName,
                                            const std::string& version) {
    m_projectName = projectName;
    m_version = version;
}

std::string PerformanceDocGenerator::generate(const std::vector<BenchmarkResult>& results,
                                             DocumentFormat format) const {
    switch (format) {
        case DocumentFormat::MARKDOWN:
            return generateMarkdown(results);
        case DocumentFormat::HTML:
            return generateHTML(results);
        case DocumentFormat::JSON:
            return generateJSON(results);
        default:
            return generateMarkdown(results);
    }
}

std::string PerformanceDocGenerator::generateMarkdown(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    // Header
    oss << generateMarkdownHeader();
    oss << "\n";

    // Summary
    oss << "## Executive Summary\n\n";
    oss << generateSummary(results);
    oss << "\n";

    // Results table
    oss << "## Benchmark Results\n\n";
    oss << generateMarkdownTable(results);
    oss << "\n";

    // Comparison
    oss << "## Performance Comparison\n\n";
    oss << generateMarkdownComparison(results);
    oss << "\n";

    // Recommendations
    oss << "## Recommendations\n\n";
    oss << generateRecommendations(results);
    oss << "\n";

    // Footer
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    oss << "---\n\n";
    oss << "*Generated on " << std::ctime(&now_c);
    oss << "by " << m_projectName << " v" << m_version << "*\n";

    return oss.str();
}

std::string PerformanceDocGenerator::generateHTML(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    oss << generateHTMLHeader();
    oss << "<body>\n";
    oss << "<div class=\"container\">\n";

    // Header
    oss << "<h1>Spatial Index Performance Report</h1>\n";
    oss << "<p class=\"subtitle\">" << m_projectName << " v" << m_version << "</p>\n\n";

    // System info
    oss << "<div class=\"system-info\">\n";
    oss << "<h2>System Information</h2>\n";
    oss << "<ul>\n";
    oss << "<li><strong>OS:</strong> " << m_os << "</li>\n";
    oss << "<li><strong>CPU:</strong> " << m_cpu << "</li>\n";
    oss << "<li><strong>Memory:</strong> " << m_memory << "</li>\n";
    oss << "</ul>\n";
    oss << "</div>\n\n";

    // Summary
    oss << "<h2>Executive Summary</h2>\n";
    oss << "<div class=\"summary\">\n";
    std::string summary = generateSummary(results);
    // Convert newlines to <br> for HTML
    size_t pos = 0;
    while ((pos = summary.find('\n', pos)) != std::string::npos) {
        summary.replace(pos, 1, "<br>\n");
        pos += 5;
    }
    oss << summary;
    oss << "</div>\n\n";

    // Results table
    oss << "<h2>Benchmark Results</h2>\n";
    oss << generateHTMLTable(results);
    oss << "\n";

    // Recommendations
    oss << "<h2>Recommendations</h2>\n";
    oss << "<div class=\"recommendations\">\n";
    std::string recommendations = generateRecommendations(results);
    pos = 0;
    while ((pos = recommendations.find('\n', pos)) != std::string::npos) {
        recommendations.replace(pos, 1, "<br>\n");
        pos += 5;
    }
    oss << recommendations;
    oss << "</div>\n\n";

    oss << "</div>\n";  // container
    oss << generateHTMLFooter();
    oss << "</body>\n</html>";

    return oss.str();
}

std::string PerformanceDocGenerator::generateJSON(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"project\": \"" << m_projectName << "\",\n";
    oss << "  \"version\": \"" << m_version << "\",\n";
    oss << "  \"systemInfo\": {\n";
    oss << "    \"os\": \"" << m_os << "\",\n";
    oss << "    \"cpu\": \"" << m_cpu << "\",\n";
    oss << "    \"memory\": \"" << m_memory << "\"\n";
    oss << "  },\n";
    oss << "  \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        oss << "    {\n";
        oss << "      \"indexType\": \"" << r.indexType << "\",\n";
        oss << "      \"operation\": \"" << r.operationType << "\",\n";
        oss << "      \"meshSize\": " << r.meshSize << ",\n";
        oss << "      \"timeMs\": " << std::fixed << std::setprecision(4) << r.timeMilliseconds << ",\n";
        oss << "      \"queryCount\": " << r.queryCount << ",\n";
        oss << "      \"avgTimeMs\": " << std::fixed << std::setprecision(6)
            << (r.queryCount > 0 ? r.timeMilliseconds / r.queryCount : 0.0) << ",\n";
        oss << "      \"resultCount\": " << r.resultCount << ",\n";
        oss << "      \"memoryKB\": " << (r.memoryBytes / 1024) << "\n";
        oss << "    }";
        if (i < results.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }

    oss << "  ]\n";
    oss << "}\n";

    return oss.str();
}

std::string PerformanceDocGenerator::generateSummary(const std::vector<BenchmarkResult>& results) const {
    if (results.empty()) {
        return "No benchmark results available.\n";
    }

    std::ostringstream oss;

    // Group by operation
    auto grouped = groupByOperation(results);

    oss << "This report presents performance benchmarks for " << grouped.size()
        << " different operation types across " << results.size() << " test configurations.\n\n";

    // Find best performers
    for (const auto& group : grouped) {
        const auto& opType = group.first;
        const auto& opResults = group.second;

        if (opResults.empty()) continue;

        // Find fastest
        auto fastest = std::min_element(opResults.begin(), opResults.end(),
            [](const auto& a, const auto& b) {
                return a.timeMilliseconds < b.timeMilliseconds;
            });

        oss << "**" << opType << "**: Best performance with **" << fastest->indexType
            << "** (" << std::fixed << std::setprecision(3) << fastest->timeMilliseconds << " ms";
        if (fastest->queryCount > 1) {
            double avgTime = fastest->timeMilliseconds / fastest->queryCount;
            oss << ", " << std::fixed << std::setprecision(6) << avgTime << " ms/query";
        }
        oss << ")\n\n";
    }

    return oss.str();
}

std::string PerformanceDocGenerator::generateRecommendations(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    auto grouped = groupByOperation(results);

    oss << "Based on benchmark results, we recommend the following spatial index types for different use cases:\n\n";

    for (const auto& group : grouped) {
        const auto& opType = group.first;
        const auto& opResults = group.second;

        if (opResults.empty()) continue;

        // Find fastest and slowest
        auto fastest = std::min_element(opResults.begin(), opResults.end(),
            [](const auto& a, const auto& b) {
                return a.timeMilliseconds < b.timeMilliseconds;
            });

        auto slowest = std::max_element(opResults.begin(), opResults.end(),
            [](const auto& a, const auto& b) {
                return a.timeMilliseconds < b.timeMilliseconds;
            });

        double speedup = slowest->timeMilliseconds / fastest->timeMilliseconds;

        oss << "- **" << opType << "**: Use **" << fastest->indexType << "**";
        if (speedup > 1.5) {
            oss << " (up to " << std::fixed << std::setprecision(1) << speedup << "x faster than "
                << slowest->indexType << ")";
        }
        oss << "\n";
    }

    oss << "\n**General Guidelines:**\n\n";
    oss << "- For point queries: Uniform Grid offers O(1) average-case performance\n";
    oss << "- For nearest neighbor: K-d Tree provides excellent spatial partitioning\n";
    oss << "- For range queries: R-Tree minimizes overlap for efficient searches\n";
    oss << "- For general use: Octree provides balanced performance across operations\n";
    oss << "- For very large meshes: Uniform Grid scales well with element count\n";

    return oss.str();
}

std::string PerformanceDocGenerator::generateMarkdownHeader() const {
    std::ostringstream oss;

    oss << "# Spatial Index Performance Report\n\n";
    oss << "**Project:** " << m_projectName << " v" << m_version << "\n\n";
    oss << "## System Information\n\n";
    oss << "- **Operating System:** " << m_os << "\n";
    oss << "- **CPU:** " << m_cpu << "\n";
    oss << "- **Memory:** " << m_memory << "\n\n";

    return oss.str();
}

std::string PerformanceDocGenerator::generateMarkdownTable(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    oss << "| Index Type | Operation | Mesh Size | Time (ms) | Avg (ms/q) | Memory (KB) | Speedup |\n";
    oss << "|------------|-----------|-----------|-----------|------------|-------------|--------:|\n";

    auto grouped = groupByOperation(results);

    for (const auto& group : grouped) {
        const auto& opResults = group.second;
        double fastestTime = getFastestTime(results, group.first);

        for (const auto& r : opResults) {
            double avgTime = r.queryCount > 0 ? r.timeMilliseconds / r.queryCount : 0.0;
            double speedup = fastestTime > 0.0 ? r.timeMilliseconds / fastestTime : 1.0;

            oss << "| " << r.indexType
                << " | " << r.operationType
                << " | " << r.meshSize
                << " | " << std::fixed << std::setprecision(3) << r.timeMilliseconds
                << " | " << std::fixed << std::setprecision(6) << avgTime
                << " | " << (r.memoryBytes / 1024)
                << " | " << std::fixed << std::setprecision(2) << speedup << "x |\n";
        }
    }

    return oss.str();
}

std::string PerformanceDocGenerator::generateMarkdownComparison(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    auto grouped = groupByOperation(results);

    for (const auto& group : grouped) {
        oss << "### " << group.first << "\n\n";

        const auto& opResults = group.second;
        double fastestTime = getFastestTime(results, group.first);

        oss << "| Index Type | Time (ms) | Relative Performance |\n";
        oss << "|------------|-----------|---------------------:|\n";

        for (const auto& r : opResults) {
            double speedup = fastestTime > 0.0 ? r.timeMilliseconds / fastestTime : 1.0;
            std::string bar(static_cast<size_t>(speedup * 10), '█');

            oss << "| " << r.indexType
                << " | " << std::fixed << std::setprecision(3) << r.timeMilliseconds
                << " | " << bar << " " << std::fixed << std::setprecision(2) << speedup << "x |\n";
        }

        oss << "\n";
    }

    return oss.str();
}

std::string PerformanceDocGenerator::generateHTMLHeader() const {
    std::ostringstream oss;

    oss << "<!DOCTYPE html>\n";
    oss << "<html lang=\"en\">\n";
    oss << "<head>\n";
    oss << "<meta charset=\"UTF-8\">\n";
    oss << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    oss << "<title>Spatial Index Performance Report - " << m_projectName << "</title>\n";
    oss << "<style>\n";
    oss << "body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; max-width: 1200px; margin: 0 auto; padding: 20px; background: #f4f4f4; }\n";
    oss << ".container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    oss << "h1 { color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }\n";
    oss << "h2 { color: #34495e; margin-top: 30px; border-bottom: 2px solid #ecf0f1; padding-bottom: 8px; }\n";
    oss << ".subtitle { color: #7f8c8d; font-size: 1.1em; margin-top: -10px; }\n";
    oss << ".system-info { background: #ecf0f1; padding: 15px; border-radius: 5px; margin: 20px 0; }\n";
    oss << ".system-info ul { list-style: none; padding: 0; }\n";
    oss << ".system-info li { padding: 5px 0; }\n";
    oss << "table { width: 100%; border-collapse: collapse; margin: 20px 0; }\n";
    oss << "th { background: #3498db; color: white; padding: 12px; text-align: left; }\n";
    oss << "td { padding: 10px; border-bottom: 1px solid #ecf0f1; }\n";
    oss << "tr:hover { background: #f8f9fa; }\n";
    oss << ".summary, .recommendations { background: #e8f4f8; padding: 15px; border-left: 4px solid #3498db; margin: 20px 0; }\n";
    oss << ".footer { margin-top: 40px; padding-top: 20px; border-top: 1px solid #ecf0f1; color: #7f8c8d; text-align: center; }\n";
    oss << "</style>\n";
    oss << "</head>\n";

    return oss.str();
}

std::string PerformanceDocGenerator::generateHTMLTable(const std::vector<BenchmarkResult>& results) const {
    std::ostringstream oss;

    oss << "<table>\n";
    oss << "<thead>\n";
    oss << "<tr>\n";
    oss << "<th>Index Type</th><th>Operation</th><th>Mesh Size</th>\n";
    oss << "<th>Time (ms)</th><th>Avg (ms/query)</th><th>Memory (KB)</th><th>Speedup</th>\n";
    oss << "</tr>\n";
    oss << "</thead>\n";
    oss << "<tbody>\n";

    auto grouped = groupByOperation(results);

    for (const auto& group : grouped) {
        const auto& opResults = group.second;
        double fastestTime = getFastestTime(results, group.first);

        for (const auto& r : opResults) {
            double avgTime = r.queryCount > 0 ? r.timeMilliseconds / r.queryCount : 0.0;
            double speedup = fastestTime > 0.0 ? r.timeMilliseconds / fastestTime : 1.0;

            oss << "<tr>\n";
            oss << "<td>" << r.indexType << "</td>\n";
            oss << "<td>" << r.operationType << "</td>\n";
            oss << "<td>" << r.meshSize << "</td>\n";
            oss << "<td>" << std::fixed << std::setprecision(3) << r.timeMilliseconds << "</td>\n";
            oss << "<td>" << std::fixed << std::setprecision(6) << avgTime << "</td>\n";
            oss << "<td>" << (r.memoryBytes / 1024) << "</td>\n";
            oss << "<td>" << std::fixed << std::setprecision(2) << speedup << "x</td>\n";
            oss << "</tr>\n";
        }
    }

    oss << "</tbody>\n";
    oss << "</table>\n";

    return oss.str();
}

std::string PerformanceDocGenerator::generateHTMLFooter() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << "<div class=\"footer\">\n";
    oss << "<p>Generated on " << std::ctime(&now_c);
    oss << "by " << m_projectName << " v" << m_version << "</p>\n";
    oss << "</div>\n";

    return oss.str();
}

bool PerformanceDocGenerator::saveToFile(const std::string& content, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << content;
    file.close();
    return true;
}

std::string PerformanceDocGenerator::getFileExtension(DocumentFormat format) {
    switch (format) {
        case DocumentFormat::MARKDOWN: return ".md";
        case DocumentFormat::HTML: return ".html";
        case DocumentFormat::JSON: return ".json";
        case DocumentFormat::LATEX: return ".tex";
        default: return ".txt";
    }
}

double PerformanceDocGenerator::getFastestTime(const std::vector<BenchmarkResult>& results,
                                              const std::string& operationType) const {
    double fastest = std::numeric_limits<double>::max();

    for (const auto& r : results) {
        if (r.operationType == operationType && r.timeMilliseconds < fastest) {
            fastest = r.timeMilliseconds;
        }
    }

    return fastest;
}

std::map<std::string, std::vector<BenchmarkResult>> PerformanceDocGenerator::groupByOperation(
    const std::vector<BenchmarkResult>& results) const {

    std::map<std::string, std::vector<BenchmarkResult>> grouped;

    for (const auto& r : results) {
        grouped[r.operationType].push_back(r);
    }

    return grouped;
}

} // namespace core
} // namespace koomesh
