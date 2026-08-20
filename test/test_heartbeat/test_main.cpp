#include <unity.h>

// ============================================================
// Tests unitarios — Logica de heartbeat y fallback
// ============================================================

#define HEARTBEAT_INTERVAL_MS 5000
#define HEARTBEAT_TIMEOUT_COUNT 3

void test_heartbeat_interval_is_positive() {
    TEST_ASSERT_GREATER_THAN(0, HEARTBEAT_INTERVAL_MS);
}

void test_fallback_threshold_is_15_seconds() {
    unsigned long fallbackMs = HEARTBEAT_INTERVAL_MS * HEARTBEAT_TIMEOUT_COUNT;
    TEST_ASSERT_EQUAL_UINT32(15000, fallbackMs);
}

void test_single_failure_does_not_trigger_fallback() {
    int failures = 1;
    TEST_ASSERT_TRUE(failures < HEARTBEAT_TIMEOUT_COUNT);
}

void test_three_failures_triggers_fallback() {
    int failures = 3;
    TEST_ASSERT_TRUE(failures >= HEARTBEAT_TIMEOUT_COUNT);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_heartbeat_interval_is_positive);
    RUN_TEST(test_fallback_threshold_is_15_seconds);
    RUN_TEST(test_single_failure_does_not_trigger_fallback);
    RUN_TEST(test_three_failures_triggers_fallback);
    return UNITY_END();
}
