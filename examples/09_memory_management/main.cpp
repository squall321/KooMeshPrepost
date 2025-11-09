/**
 * @file main.cpp
 * @brief Memory Management 사용 예제
 *
 * 메모리 풀, 스마트 포인터, RAII 등 메모리 관리 기법을 시연합니다.
 */

#include "utils/MemoryPool.h"
#include "utils/Logger.h"
#include "core/Node.h"
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

using namespace koomesh::utils;
using namespace koomesh::core;

// ======================================================================
// 헬퍼 함수
// ======================================================================

void printSeparator(const std::string& title) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

// ======================================================================
// 예제 1: RAII 패턴
// ======================================================================

class FileHandle {
public:
    FileHandle(const std::string& filename) : m_filename(filename) {
        Logger::infof("Opening file: {}", filename);
        // 실제로는 파일을 열겠지만 여기서는 시뮬레이션
        m_isOpen = true;
    }

    ~FileHandle() {
        if (m_isOpen) {
            Logger::infof("Closing file: {}", m_filename);
            m_isOpen = false;
        }
    }

    // 복사 방지
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 이동 허용
    FileHandle(FileHandle&& other) noexcept
        : m_filename(std::move(other.m_filename))
        , m_isOpen(other.m_isOpen) {
        other.m_isOpen = false;
    }

    bool isOpen() const { return m_isOpen; }

private:
    std::string m_filename;
    bool m_isOpen;
};

void example1_RAII() {
    printSeparator("Example 1: RAII 패턴");

    {
        FileHandle file("data.txt");
        std::cout << "File is " << (file.isOpen() ? "open" : "closed") << "\n";

        // 예외가 발생해도 파일은 자동으로 닫힘
        // 스코프를 벗어나면 소멸자가 자동 호출됨
    }  // 여기서 file 소멸자 호출

    std::cout << "File has been automatically closed\n";
}

// ======================================================================
// 예제 2: 스마트 포인터 - unique_ptr
// ======================================================================

void example2_UniquePtr() {
    printSeparator("Example 2: unique_ptr");

    // unique_ptr로 독점 소유권
    auto node1 = std::make_unique<Node>(1, 0.0, 0.0, 0.0);
    Logger::infof("Created node {}", node1->id());

    // 소유권 이전
    std::unique_ptr<Node> node2 = std::move(node1);
    std::cout << "node1 is " << (node1 ? "valid" : "null") << "\n";
    std::cout << "node2 is " << (node2 ? "valid" : "null") << "\n";

    // 벡터에 저장 (소유권 이전)
    std::vector<std::unique_ptr<Node>> nodes;
    nodes.push_back(std::make_unique<Node>(2, 1.0, 0.0, 0.0));
    nodes.push_back(std::make_unique<Node>(3, 0.0, 1.0, 0.0));
    nodes.push_back(std::move(node2));

    Logger::infof("Vector has {} nodes", nodes.size());

    // 스코프를 벗어나면 모든 노드 자동 삭제
}

// ======================================================================
// 예제 3: 스마트 포인터 - shared_ptr
// ======================================================================

class Resource {
public:
    Resource(int id) : m_id(id) {
        Logger::infof("Resource {} created", m_id);
    }

    ~Resource() {
        Logger::infof("Resource {} destroyed", m_id);
    }

    int id() const { return m_id; }

private:
    int m_id;
};

void example3_SharedPtr() {
    printSeparator("Example 3: shared_ptr");

    std::shared_ptr<Resource> res1 = std::make_shared<Resource>(1);
    std::cout << "res1 use_count: " << res1.use_count() << "\n";

    {
        std::shared_ptr<Resource> res2 = res1;  // 참조 카운트 증가
        std::cout << "res1 use_count: " << res1.use_count() << "\n";
        std::cout << "res2 use_count: " << res2.use_count() << "\n";

        std::shared_ptr<Resource> res3 = res1;
        std::cout << "res1 use_count: " << res1.use_count() << "\n";
    }  // res2, res3 소멸 -> 참조 카운트 감소

    std::cout << "res1 use_count: " << res1.use_count() << "\n";

    // res1이 소멸될 때 Resource 객체 삭제
}

// ======================================================================
// 예제 4: MemoryPool 기본 사용
// ======================================================================

void example4_MemoryPoolBasic() {
    printSeparator("Example 4: MemoryPool 기본 사용");

    MemoryPool<Node> nodePool;

    Logger::info("Allocating nodes from pool...");

    // 풀에서 할당
    std::vector<Node*> nodes;
    for (int i = 1; i <= 10; ++i) {
        Node* node = nodePool.construct(i, i * 1.0, i * 2.0, i * 3.0);
        nodes.push_back(node);
    }

    Logger::infof("Active nodes: {}", nodePool.activeCount());
    Logger::infof("Total allocated: {}", nodePool.totalAllocated());
    Logger::infof("Memory blocks: {}", nodePool.blockCount());
    Logger::infof("Memory usage: {} bytes", nodePool.memoryUsage());

    // 노드 사용
    std::cout << "\nNode positions:\n";
    for (const auto* node : nodes) {
        std::cout << "  Node " << node->id() << ": ("
                  << node->x() << ", " << node->y() << ", " << node->z() << ")\n";
    }

    // 풀로 반환
    Logger::info("\nDeallocating nodes...");
    for (auto* node : nodes) {
        nodePool.destroy(node);
    }

    Logger::infof("Active nodes after deallocation: {}", nodePool.activeCount());
}

// ======================================================================
// 예제 5: MemoryPool 재사용
// ======================================================================

void example5_MemoryPoolReuse() {
    printSeparator("Example 5: MemoryPool 재사용");

    MemoryPool<Node> nodePool;

    Logger::info("Allocation-deallocation cycle:");

    for (int cycle = 1; cycle <= 3; ++cycle) {
        std::cout << "\nCycle " << cycle << ":\n";

        // 할당
        std::vector<Node*> nodes;
        for (int i = 1; i <= 5; ++i) {
            nodes.push_back(nodePool.construct(cycle * 10 + i, 0.0, 0.0, 0.0));
        }

        std::cout << "  Allocated: " << nodePool.activeCount() << " nodes\n";
        std::cout << "  Blocks: " << nodePool.blockCount() << "\n";

        // 해제
        for (auto* node : nodes) {
            nodePool.destroy(node);
        }

        std::cout << "  After deallocation: " << nodePool.activeCount() << " nodes\n";
    }

    Logger::infof("Total allocations: {}", nodePool.totalAllocated());
    Logger::infof("Total deallocations: {}", nodePool.totalDeallocated());
}

// ======================================================================
// 예제 6: 성능 비교 - MemoryPool vs new/delete
// ======================================================================

void example6_PerformanceComparison() {
    printSeparator("Example 6: 성능 비교");

    const size_t iterations = 10000;

    // MemoryPool 사용
    {
        MemoryPool<Node> pool;
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            Node* node = pool.construct(i, 0.0, 0.0, 0.0);
            pool.destroy(node);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "MemoryPool:  " << duration.count() << " μs\n";
    }

    // new/delete 사용
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            Node* node = new Node(i, 0.0, 0.0, 0.0);
            delete node;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "new/delete:  " << duration.count() << " μs\n";
    }

    std::cout << "\n메모리 풀이 일반적으로 더 빠릅니다!\n";
}

// ======================================================================
// 예제 7: 벡터 reserve로 재할당 방지
// ======================================================================

void example7_VectorReserve() {
    printSeparator("Example 7: Vector Reserve");

    // Reserve 없이
    {
        std::cout << "Without reserve:\n";
        std::vector<int> vec;
        size_t reallocations = 0;
        size_t lastCapacity = 0;

        for (int i = 0; i < 100; ++i) {
            vec.push_back(i);
            if (vec.capacity() != lastCapacity) {
                std::cout << "  Reallocation at size " << vec.size()
                          << ", new capacity: " << vec.capacity() << "\n";
                ++reallocations;
                lastCapacity = vec.capacity();
            }
        }

        std::cout << "Total reallocations: " << reallocations << "\n";
    }

    // Reserve 사용
    {
        std::cout << "\nWith reserve:\n";
        std::vector<int> vec;
        vec.reserve(100);  // 미리 공간 확보

        size_t reallocations = 0;
        size_t lastCapacity = vec.capacity();

        for (int i = 0; i < 100; ++i) {
            vec.push_back(i);
            if (vec.capacity() != lastCapacity) {
                std::cout << "  Reallocation at size " << vec.size()
                          << ", new capacity: " << vec.capacity() << "\n";
                ++reallocations;
                lastCapacity = vec.capacity();
            }
        }

        std::cout << "Total reallocations: " << reallocations << "\n";
    }
}

// ======================================================================
// 예제 8: 이동 의미론
// ======================================================================

class LargeData {
public:
    LargeData(size_t size) : m_size(size) {
        m_data = new int[size];
        Logger::infof("LargeData constructed (size: {})", size);
    }

    ~LargeData() {
        if (m_data) {
            Logger::infof("LargeData destroyed (size: {})", m_size);
            delete[] m_data;
        }
    }

    // 복사 생성자 (비용이 큼)
    LargeData(const LargeData& other) : m_size(other.m_size) {
        m_data = new int[m_size];
        std::copy(other.m_data, other.m_data + m_size, m_data);
        Logger::infof("LargeData copied (size: {})", m_size);
    }

    // 이동 생성자 (비용이 적음)
    LargeData(LargeData&& other) noexcept : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
        Logger::infof("LargeData moved (size: {})", m_size);
    }

    // 이동 대입
    LargeData& operator=(LargeData&& other) noexcept {
        if (this != &other) {
            delete[] m_data;
            m_data = other.m_data;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
            Logger::info("LargeData move-assigned");
        }
        return *this;
    }

    size_t size() const { return m_size; }

private:
    int* m_data;
    size_t m_size;
};

void example8_MoveSemantics() {
    printSeparator("Example 8: 이동 의미론");

    std::cout << "Creating large data...\n";
    LargeData data1(1000000);

    std::cout << "\nMoving data (cheap)...\n";
    LargeData data2 = std::move(data1);  // 이동 생성

    std::cout << "\ndata1 size: " << data1.size() << " (moved from)\n";
    std::cout << "data2 size: " << data2.size() << "\n";

    std::cout << "\nCopying would be expensive, moving is fast!\n";
}

// ======================================================================
// 메인 함수
// ======================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  Memory Management Examples\n";
    std::cout << "========================================\n";

    Logger::getInstance().setConsoleOutput(true);
    Logger::getInstance().setLevel(LogLevel::INFO);

    try {
        example1_RAII();
        example2_UniquePtr();
        example3_SharedPtr();
        example4_MemoryPoolBasic();
        example5_MemoryPoolReuse();
        example6_PerformanceComparison();
        example7_VectorReserve();
        example8_MoveSemantics();

        printSeparator("모든 예제 완료");
        std::cout << "✓ 모든 예제가 성공적으로 실행되었습니다.\n";

    } catch (const std::exception& e) {
        std::cerr << "✗ 오류 발생: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
