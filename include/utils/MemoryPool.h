#pragma once

#include <cstddef>
#include <vector>
#include <algorithm>
#include <new>

namespace koomesh {
namespace utils {

/**
 * @brief 메모리 풀 - 고정 크기 객체의 효율적인 할당/해제
 *
 * 작은 객체를 대량으로 할당/해제하는 경우 일반 new/delete보다
 * 훨씬 빠르고 메모리 단편화를 줄입니다.
 *
 * Example usage:
 * @code
 * MemoryPool<Node> nodePool;
 *
 * // 할당
 * Node* node = nodePool.allocate();
 * new (node) Node(1, 0.0, 0.0, 0.0);  // Placement new
 *
 * // 사용
 * node->setPosition(1.0, 2.0, 3.0);
 *
 * // 해제
 * node->~Node();  // 소멸자 명시적 호출
 * nodePool.deallocate(node);
 * @endcode
 *
 * @tparam T 할당할 객체 타입
 * @tparam BlockSize 한 번에 할당할 메모리 블록 크기 (바이트)
 */
template<typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    /**
     * @brief 생성자
     */
    MemoryPool() : m_freeList(nullptr), m_allocatedCount(0), m_deallocatedCount(0) {}

    /**
     * @brief 소멸자 - 모든 블록 해제
     */
    ~MemoryPool() {
        for (auto block : m_blocks) {
            ::operator delete(block);
        }
    }

    // 복사 방지
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // 이동 허용
    MemoryPool(MemoryPool&& other) noexcept
        : m_freeList(other.m_freeList)
        , m_blocks(std::move(other.m_blocks))
        , m_allocatedCount(other.m_allocatedCount)
        , m_deallocatedCount(other.m_deallocatedCount) {
        other.m_freeList = nullptr;
        other.m_allocatedCount = 0;
        other.m_deallocatedCount = 0;
    }

    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            // 기존 블록 해제
            for (auto block : m_blocks) {
                ::operator delete(block);
            }

            m_freeList = other.m_freeList;
            m_blocks = std::move(other.m_blocks);
            m_allocatedCount = other.m_allocatedCount;
            m_deallocatedCount = other.m_deallocatedCount;

            other.m_freeList = nullptr;
            other.m_allocatedCount = 0;
            other.m_deallocatedCount = 0;
        }
        return *this;
    }

    /**
     * @brief 메모리 할당
     *
     * @return 할당된 메모리 포인터 (생성자는 호출되지 않음)
     * @throws std::bad_alloc 메모리 부족 시
     */
    T* allocate() {
        if (!m_freeList) {
            allocateBlock();
        }

        T* result = reinterpret_cast<T*>(m_freeList);
        m_freeList = m_freeList->next;
        ++m_allocatedCount;

        return result;
    }

    /**
     * @brief 메모리 해제
     *
     * @param ptr 해제할 메모리 포인터 (소멸자는 호출되지 않음)
     *
     * @note 소멸자는 호출자가 직접 호출해야 합니다.
     */
    void deallocate(T* ptr) {
        if (!ptr) {
            return;
        }

        FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = m_freeList;
        m_freeList = node;
        ++m_deallocatedCount;
    }

    /**
     * @brief 객체 생성 (할당 + 생성자 호출)
     *
     * @tparam Args 생성자 인자 타입
     * @param args 생성자 인자
     * @return 생성된 객체 포인터
     */
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        try {
            new (ptr) T(std::forward<Args>(args)...);
            return ptr;
        } catch (...) {
            deallocate(ptr);
            throw;
        }
    }

    /**
     * @brief 객체 파괴 (소멸자 호출 + 해제)
     *
     * @param ptr 파괴할 객체 포인터
     */
    void destroy(T* ptr) {
        if (!ptr) {
            return;
        }

        ptr->~T();
        deallocate(ptr);
    }

    /**
     * @brief 할당된 객체 수 반환
     *
     * @return 현재 활성 객체 수 (할당 - 해제)
     */
    size_t activeCount() const {
        return m_allocatedCount - m_deallocatedCount;
    }

    /**
     * @brief 총 할당 횟수 반환
     *
     * @return 총 할당 횟수
     */
    size_t totalAllocated() const {
        return m_allocatedCount;
    }

    /**
     * @brief 총 해제 횟수 반환
     *
     * @return 총 해제 횟수
     */
    size_t totalDeallocated() const {
        return m_deallocatedCount;
    }

    /**
     * @brief 할당된 블록 수 반환
     *
     * @return 블록 수
     */
    size_t blockCount() const {
        return m_blocks.size();
    }

    /**
     * @brief 총 메모리 사용량 반환 (바이트)
     *
     * @return 총 메모리 사용량
     */
    size_t memoryUsage() const {
        return m_blocks.size() * BlockSize;
    }

private:
    /**
     * @brief Free list 노드
     */
    struct FreeNode {
        FreeNode* next;
    };

    /**
     * @brief 새로운 메모리 블록 할당
     */
    void allocateBlock() {
        // 객체 크기와 FreeNode 크기 중 큰 것 사용
        constexpr size_t objectSize = sizeof(T);
        constexpr size_t nodeSize = sizeof(FreeNode);
        constexpr size_t allocationSize = (objectSize > nodeSize) ? objectSize : nodeSize;

        // 정렬 고려
        constexpr size_t alignment = alignof(T);
        constexpr size_t alignedSize = ((allocationSize + alignment - 1) / alignment) * alignment;

        // 블록 당 객체 수 계산
        const size_t objectsPerBlock = BlockSize / alignedSize;

        if (objectsPerBlock == 0) {
            throw std::bad_alloc();
        }

        // 메모리 블록 할당
        char* block = static_cast<char*>(::operator new(BlockSize));
        m_blocks.push_back(block);

        // Free list 구축
        for (size_t i = 0; i < objectsPerBlock; ++i) {
            FreeNode* node = reinterpret_cast<FreeNode*>(block + i * alignedSize);
            node->next = m_freeList;
            m_freeList = node;
        }
    }

private:
    FreeNode* m_freeList;              ///< Free list 헤드
    std::vector<void*> m_blocks;       ///< 할당된 메모리 블록 목록
    size_t m_allocatedCount;           ///< 총 할당 횟수
    size_t m_deallocatedCount;         ///< 총 해제 횟수
};

/**
 * @brief 메모리 풀 할당자 - STL 컨테이너와 함께 사용
 *
 * Example usage:
 * @code
 * MemoryPool<int> pool;
 * PoolAllocator<int> allocator(pool);
 * std::vector<int, PoolAllocator<int>> vec(allocator);
 * @endcode
 *
 * @tparam T 할당할 객체 타입
 */
template<typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };

    PoolAllocator(MemoryPool<T>& pool) : m_pool(&pool) {}

    template<typename U>
    PoolAllocator(const PoolAllocator<U>& other) : m_pool(other.m_pool) {}

    pointer allocate(size_type n) {
        if (n != 1) {
            // 풀은 단일 객체만 할당
            return static_cast<pointer>(::operator new(n * sizeof(T)));
        }
        return m_pool->allocate();
    }

    void deallocate(pointer p, size_type n) {
        if (n != 1) {
            ::operator delete(p);
        } else {
            m_pool->deallocate(p);
        }
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new (p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }

private:
    MemoryPool<T>* m_pool;

    template<typename U>
    friend class PoolAllocator;
};

template<typename T, typename U>
bool operator==(const PoolAllocator<T>&, const PoolAllocator<U>&) {
    return true;
}

template<typename T, typename U>
bool operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&) {
    return false;
}

} // namespace utils
} // namespace koomesh
