#ifndef MOCK_UNITY_H
#define MOCK_UNITY_H

#include <iostream>
#include <cstdint>
#include <cassert>

static int g_unity_tests_run = 0;
static int g_unity_tests_failed = 0;

inline void UNITY_BEGIN() {
    g_unity_tests_run = 0;
    g_unity_tests_failed = 0;
}

inline int UNITY_END() {
    return g_unity_tests_failed;
}

void setUp(void);
void tearDown(void);

#define RUN_TEST(func) do { \
    std::cout << "[TEST] " << #func << "..."; \
    setUp(); \
    func(); \
    tearDown(); \
    g_unity_tests_run++; \
    std::cout << " PASSED\n"; \
} while(0)

#define TEST_ASSERT_TRUE(c) assert((c))
#define TEST_ASSERT_FALSE(c) assert(!(c))
#define TEST_ASSERT_EQUAL(e, a) assert((e) == (a))
#define TEST_ASSERT_EQUAL_UINT16(e, a) assert((uint16_t)(e) == (uint16_t)(a))
#define TEST_ASSERT_EQUAL_UINT32(e, a) assert((uint32_t)(e) == (uint32_t)(a))
#define TEST_ASSERT_NOT_NULL(p) assert((p) != nullptr)
#define TEST_ASSERT_NULL(p) assert((p) == nullptr)

#endif // MOCK_UNITY_H
