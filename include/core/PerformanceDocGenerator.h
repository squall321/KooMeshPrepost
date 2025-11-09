#pragma once

#include "core/SpatialIndexBenchmark.h"
#include <string>
#include <vector>
#include <map>

namespace koomesh {
namespace core {

/**
 * @brief Format for documentation output
 */
enum class DocumentFormat {
    MARKDOWN,    ///< Markdown format (.md)
    HTML,        ///< HTML format (.html)
    JSON,        ///< JSON format (.json)
    LATEX        ///< LaTeX format (.tex)
};

/**
 * @brief Generator for performance documentation from benchmark results
 *
 * This class generates comprehensive performance documentation including:
 * - Benchmark result tables
 * - Performance comparison charts
 * - Recommendation summaries
 * - System specifications
 * - Usage guidelines
 *
 * Example usage:
 * @code
 * PerformanceDocGenerator generator;
 *
 * // Set system info
 * generator.setSystemInfo("Ubuntu 22.04", "Intel i7-9700K", "32GB RAM");
 *
 * // Generate markdown documentation
 * std::string markdown = generator.generateMarkdown(benchmarkResults);
 * generator.saveToFile(markdown, "performance_report.md");
 * @endcode
 */
class PerformanceDocGenerator {
public:
    /**
     * @brief Constructor
     */
    PerformanceDocGenerator();

    /**
     * @brief Set system information for documentation
     * @param os Operating system
     * @param cpu CPU information
     * @param memory Memory information
     */
    void setSystemInfo(const std::string& os,
                      const std::string& cpu,
                      const std::string& memory);

    /**
     * @brief Set project information
     * @param projectName Project name
     * @param version Project version
     */
    void setProjectInfo(const std::string& projectName,
                       const std::string& version);

    /**
     * @brief Generate documentation in Markdown format
     * @param results Benchmark results
     * @return Markdown-formatted documentation
     */
    std::string generateMarkdown(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate documentation in HTML format
     * @param results Benchmark results
     * @return HTML-formatted documentation
     */
    std::string generateHTML(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate documentation in JSON format
     * @param results Benchmark results
     * @return JSON-formatted documentation
     */
    std::string generateJSON(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate documentation in specified format
     * @param results Benchmark results
     * @param format Output format
     * @return Formatted documentation
     */
    std::string generate(const std::vector<BenchmarkResult>& results,
                        DocumentFormat format) const;

    /**
     * @brief Save documentation to file
     * @param content Documentation content
     * @param filename Output filename
     * @return True if successful, false otherwise
     */
    static bool saveToFile(const std::string& content, const std::string& filename);

    /**
     * @brief Generate performance summary section
     * @param results Benchmark results
     * @return Summary text
     */
    std::string generateSummary(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate recommendations section
     * @param results Benchmark results
     * @return Recommendations text
     */
    std::string generateRecommendations(const std::vector<BenchmarkResult>& results) const;

private:
    /**
     * @brief Generate header section for markdown
     */
    std::string generateMarkdownHeader() const;

    /**
     * @brief Generate results table in markdown
     */
    std::string generateMarkdownTable(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate comparison section in markdown
     */
    std::string generateMarkdownComparison(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate HTML header with CSS
     */
    std::string generateHTMLHeader() const;

    /**
     * @brief Generate HTML table
     */
    std::string generateHTMLTable(const std::vector<BenchmarkResult>& results) const;

    /**
     * @brief Generate HTML footer
     */
    std::string generateHTMLFooter() const;

    /**
     * @brief Get file extension for format
     */
    static std::string getFileExtension(DocumentFormat format);

    /**
     * @brief Get fastest time for operation type
     */
    double getFastestTime(const std::vector<BenchmarkResult>& results,
                         const std::string& operationType) const;

    /**
     * @brief Group results by operation type
     */
    std::map<std::string, std::vector<BenchmarkResult>> groupByOperation(
        const std::vector<BenchmarkResult>& results) const;

    std::string m_os;
    std::string m_cpu;
    std::string m_memory;
    std::string m_projectName;
    std::string m_version;
};

} // namespace core
} // namespace koomesh
