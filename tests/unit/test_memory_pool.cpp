#include <gtest/gtest.h>
#include "utils/MemoryPool.h"
#include <vector>
#include <thread>

using namespace koomesh::utils;

// 테스트용 간단한 구조체
struct TestObject {
    int value;
    double data;

    TestObject() : value(0), data(0.0) {}
    TestObject(int v, double d) : value(v), data(d) {}

    ~TestObject() {
        // 소멸자 호출 확인용
        value = -1;
    }
};

// ======================================================================
// MemoryPool 기본 테스트
// ======================================================================

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MemoryPoolTest, BasicAllocation) {
    MemoryPool<TestObject> pool;

    // 할당
    TestObject* obj = pool.allocate();
    ASSERT_NE(nullptr, obj);

    // Placement new로 생성자 호출
    new (obj) TestObject(42, 3.14);

    EXPECT_EQ(42, obj->value);
    EXPECT_DOUBLE_EQ(3.14, obj->data);

    // 소멸자 호출 및 해제
    obj->~TestObject();
    pool.deallocate(obj);
}

TEST_F(MemoryPoolTest, MultipleAllocations) {
    MemoryPool<TestObject> pool;

    std::vector<TestObject*> objects;
    const size_t count = 100;

    // 다중 할당
    for (size_t i = 0; i < count; ++i) {
        TestObject* obj = pool.allocate();
        new (obj) TestObject(static_cast<int>(i), static_cast<double>(i) * 2.0);
        objects.push_back(obj);
    }

    EXPECT_EQ(count, pool.activeCount());
    EXPECT_EQ(count, pool.totalAllocated());

    // 검증
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(static_cast<int>(i), objects[i]->value);
        EXPECT_DOUBLE_EQ(static_cast<double>(i) * 2.0, objects[i]->data);
    }

    // 해제
    for (auto obj : objects) {
        obj->~TestObject();
        pool.deallocate(obj);
    }

    EXPECT_EQ(0, pool.activeCount());
    EXPECT_EQ(count, pool.totalDeallocated());
}

TEST_F(MemoryPoolTest, ConstructAndDestroy) {
    MemoryPool<TestObject> pool;

    // construct 메서드 사용 (할당 + 생성자 호출)
    TestObject* obj = pool.construct(123, 4.56);

    ASSERT_NE(nullptr, obj);
    EXPECT_EQ(123, obj->value);
    EXPECT_DOUBLE_EQ(4.56, obj->data);

    // destroy 메서드 사용 (소멸자 호출 + 해제)
    pool.destroy(obj);

    EXPECT_EQ(0, pool.activeCount());
}

TEST_F(MemoryPoolTest, MultipleConstructDestroy) {
    MemoryPool<TestObject> pool;

    std::vector<TestObject*> objects;
    const size_t count = 50;

    // construct
    for (size_t i = 0; i < count; ++i) {
        objects.push_back(pool.construct(static_cast<int>(i), static_cast<double>(i)));
    }

    EXPECT_EQ(count, pool.activeCount());

    // destroy
    for (auto obj : objects) {
        pool.destroy(obj);
    }

    EXPECT_EQ(0, pool.activeCount());
}

TEST_F(MemoryPoolTest, AllocationDeallocationCycle) {
    MemoryPool<TestObject> pool;

    // 할당-해제 사이클
    for (int cycle = 0; cycle < 5; ++cycle) {
        std::vector<TestObject*> objects;

        // 할당
        for (int i = 0; i < 20; ++i) {
            objects.push_back(pool.construct(i, static_cast<double>(i)));
        }

        EXPECT_EQ(20, pool.activeCount());

        // 해제
        for (auto obj : objects) {
            pool.destroy(obj);
        }

        EXPECT_EQ(0, pool.activeCount());
    }

    // 총 할당/해제 횟수 확인
    EXPECT_EQ(100, pool.totalAllocated());
    EXPECT_EQ(100, pool.totalDeallocated());
}

TEST_F(MemoryPoolTest, BlockAllocation) {
    MemoryPool<TestObject, 1024> pool;  // 1KB 블록

    // 초기 블록 수
    EXPECT_EQ(0, pool.blockCount());

    // 첫 번째 할당 시 블록 생성
    TestObject* obj1 = pool.allocate();
    EXPECT_GE(pool.blockCount(), 1);

    size_t initialBlocks = pool.blockCount();

    // 많은 객체 할당 (여러 블록 필요)
    std::vector<TestObject*> objects;
    for (int i = 0; i < 100; ++i) {
        objects.push_back(pool.allocate());
    }

    // 블록 수가 증가했는지 확인
    EXPECT_GE(pool.blockCount(), initialBlocks);

    // 정리
    pool.deallocate(obj1);
    for (auto obj : objects) {
        pool.deallocate(obj);
    }
}

TEST_F(MemoryPoolTest, MemoryUsage) {
    MemoryPool<TestObject, 4096> pool;

    // 메모리 사용량 초기값
    EXPECT_EQ(0, pool.memoryUsage());

    // 첫 번째 할당
    TestObject* obj = pool.allocate();
    EXPECT_EQ(4096, pool.memoryUsage());  // 1개 블록

    // 더 많이 할당
    std::vector<TestObject*> objects;
    for (int i = 0; i < 500; ++i) {
        objects.push_back(pool.allocate());
    }

    // 메모리 사용량은 블록 크기의 배수
    EXPECT_EQ(pool.blockCount() * 4096, pool.memoryUsage());

    // 정리
    pool.deallocate(obj);
    for (auto o : objects) {
        pool.deallocate(o);
    }
}

// ======================================================================
// 이동 의미론 테스트
// ======================================================================

TEST_F(MemoryPoolTest, MoveConstruction) {
    MemoryPool<TestObject> pool1;

    // pool1에 객체 할당
    TestObject* obj1 = pool1.construct(42, 3.14);
    size_t count1 = pool1.activeCount();

    // 이동 생성
    MemoryPool<TestObject> pool2(std::move(pool1));

    // pool2가 pool1의 상태를 가져감
    EXPECT_EQ(count1, pool2.activeCount());

    // pool1은 초기화됨
    EXPECT_EQ(0, pool1.activeCount());

    // pool2에서 정리
    pool2.destroy(obj1);
}

TEST_F(MemoryPoolTest, MoveAssignment) {
    MemoryPool<TestObject> pool1;
    MemoryPool<TestObject> pool2;

    // pool1에 객체 할당
    pool1.construct(42, 3.14);
    size_t count1 = pool1.activeCount();

    // pool2에도 객체 할당
    pool2.construct(100, 2.71);

    // 이동 대입
    pool2 = std::move(pool1);

    // pool2가 pool1의 상태를 가져감
    EXPECT_EQ(count1, pool2.activeCount());

    // pool1은 초기화됨
    EXPECT_EQ(0, pool1.activeCount());
}

// ======================================================================
// 스레드 안전성 테스트 (참고: MemoryPool 자체는 스레드 안전하지 않음)
// ======================================================================

TEST_F(MemoryPoolTest, SingleThreadedUsage) {
    // MemoryPool은 단일 스레드용으로 설계됨
    // 멀티스레드 환경에서는 각 스레드가 자신의 풀을 가져야 함

    MemoryPool<TestObject> pool;

    auto worker = [&pool]() {
        std::vector<TestObject*> localObjects;

        for (int i = 0; i < 50; ++i) {
            localObjects.push_back(pool.construct(i, static_cast<double>(i)));
        }

        for (auto obj : localObjects) {
            pool.destroy(obj);
        }
    };

    // 단일 스레드에서 실행
    worker();

    EXPECT_EQ(0, pool.activeCount());
}

// ======================================================================
// 엣지 케이스 테스트
// ======================================================================

TEST_F(MemoryPoolTest, NullptrDeallocate) {
    MemoryPool<TestObject> pool;

    // nullptr 해제는 안전해야 함
    EXPECT_NO_THROW(pool.deallocate(nullptr));
    EXPECT_NO_THROW(pool.destroy(nullptr));
}

TEST_F(MemoryPoolTest, LargeObject) {
    struct LargeObject {
        char data[1024];
        int value;

        LargeObject() : value(0) {}
    };

    MemoryPool<LargeObject> pool;

    // 큰 객체도 할당 가능
    LargeObject* obj = pool.construct();
    ASSERT_NE(nullptr, obj);

    obj->value = 42;
    EXPECT_EQ(42, obj->value);

    pool.destroy(obj);
}

TEST_F(MemoryPoolTest, SmallBlockSize) {
    // 매우 작은 블록 크기 (극단적인 경우)
    MemoryPool<TestObject, 64> pool;

    std::vector<TestObject*> objects;

    // 여러 블록 필요
    for (int i = 0; i < 10; ++i) {
        objects.push_back(pool.construct(i, static_cast<double>(i)));
    }

    EXPECT_GE(pool.blockCount(), 1);

    for (auto obj : objects) {
        pool.destroy(obj);
    }
}

// ======================================================================
// 성능 벤치마크 (참고용)
// ======================================================================

TEST_F(MemoryPoolTest, DISABLED_PerformanceBenchmark) {
    const size_t iterations = 100000;

    // MemoryPool 사용
    {
        MemoryPool<TestObject> pool;
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            TestObject* obj = pool.construct(static_cast<int>(i), static_cast<double>(i));
            pool.destroy(obj);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "MemoryPool: " << duration.count() << " ms\n";
    }

    // new/delete 사용
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            TestObject* obj = new TestObject(static_cast<int>(i), static_cast<double>(i));
            delete obj;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "new/delete: " << duration.count() << " ms\n";
    }
}

// ======================================================================
// 메인 함수
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
