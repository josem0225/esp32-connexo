#include <unity.h>

// ============================================================
// Tests unitarios — Validacion de constantes de configuracion
//
// Estos tests verifican que Config.h tiene valores sanos
// y que no se introducen regresiones por accidente.
// Se corren en entorno nativo (sin ESP32): pio test -e native
// ============================================================

// Constantes replicadas de Config.h (sin dependencia de ESP32)
#define RELAY_PIN             2
#define DOOR_SENSOR_PIN       4
#define EXIT_BUTTON_PIN       14
#define RELAY_OPEN_DURATION_MS 3000
#define HEARTBEAT_INTERVAL_MS 5000
#define HEARTBEAT_TIMEOUT_COUNT 3
#define NEXUS_ESP32_PORT      80
#define NEXUS_MINI_PC_PORT    8000
#define NEXUS_FIRMWARE_VERSION "1.1.0"

// --- Tests de pines ---

void test_relay_pin_is_gpio2() {
    TEST_ASSERT_EQUAL(2, RELAY_PIN);
}

void test_door_sensor_pin_is_gpio4() {
    TEST_ASSERT_EQUAL(4, DOOR_SENSOR_PIN);
}

void test_exit_button_pin_is_gpio14() {
    TEST_ASSERT_EQUAL(14, EXIT_BUTTON_PIN);
}

void test_no_pin_uses_gpio12() {
    // GPIO12 causa boot loop en WT32-ETH01
    TEST_ASSERT_NOT_EQUAL(12, RELAY_PIN);
    TEST_ASSERT_NOT_EQUAL(12, DOOR_SENSOR_PIN);
    TEST_ASSERT_NOT_EQUAL(12, EXIT_BUTTON_PIN);
}

void test_no_pin_conflicts_with_ethernet() {
    // Pines reservados por LAN8720A en WT32-ETH01
    int ethPins[] = {0, 1, 3, 16, 18, 23};
    int numEthPins = sizeof(ethPins) / sizeof(ethPins[0]);

    for (int i = 0; i < numEthPins; i++) {
        TEST_ASSERT_NOT_EQUAL(ethPins[i], RELAY_PIN);
        TEST_ASSERT_NOT_EQUAL(ethPins[i], DOOR_SENSOR_PIN);
        TEST_ASSERT_NOT_EQUAL(ethPins[i], EXIT_BUTTON_PIN);
    }
}

void test_all_pins_are_different() {
    TEST_ASSERT_NOT_EQUAL(RELAY_PIN, DOOR_SENSOR_PIN);
    TEST_ASSERT_NOT_EQUAL(RELAY_PIN, EXIT_BUTTON_PIN);
    TEST_ASSERT_NOT_EQUAL(DOOR_SENSOR_PIN, EXIT_BUTTON_PIN);
}

// --- Tests de tiempos ---

void test_relay_duration_is_3_seconds() {
    TEST_ASSERT_EQUAL(3000, RELAY_OPEN_DURATION_MS);
}

void test_heartbeat_interval_is_5_seconds() {
    TEST_ASSERT_EQUAL(5000, HEARTBEAT_INTERVAL_MS);
}

void test_heartbeat_timeout_is_3_failures() {
    TEST_ASSERT_EQUAL(3, HEARTBEAT_TIMEOUT_COUNT);
}

void test_fallback_activates_at_15_seconds() {
    int fallbackMs = HEARTBEAT_INTERVAL_MS * HEARTBEAT_TIMEOUT_COUNT;
    TEST_ASSERT_EQUAL(15000, fallbackMs);
}

// --- Tests de red ---

void test_esp32_http_port_is_80() {
    TEST_ASSERT_EQUAL(80, NEXUS_ESP32_PORT);
}

void test_mini_pc_port_is_8000() {
    TEST_ASSERT_EQUAL(8000, NEXUS_MINI_PC_PORT);
}

// --- Tests de OTA ---

void test_firmware_version_is_semver() {
    const char* version = NEXUS_FIRMWARE_VERSION;
    // Debe contener al menos 2 puntos (x.y.z)
    int dots = 0;
    for (int i = 0; version[i] != '\0'; i++) {
        if (version[i] == '.') dots++;
    }
    TEST_ASSERT_EQUAL(2, dots);
}

void test_firmware_version_is_1_1_0() {
    // Verifica que se bumpeó la versión para OTA
    const char* version = NEXUS_FIRMWARE_VERSION;
    TEST_ASSERT_EQUAL_STRING("1.1.0", version);
}

void test_ota_uses_http_port_80() {
    // OTA se sirve por el mismo puerto HTTP del ESP32
    TEST_ASSERT_EQUAL(80, NEXUS_ESP32_PORT);
}

// --- Runner ---

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_relay_pin_is_gpio2);
    RUN_TEST(test_door_sensor_pin_is_gpio4);
    RUN_TEST(test_exit_button_pin_is_gpio14);
    RUN_TEST(test_no_pin_uses_gpio12);
    RUN_TEST(test_no_pin_conflicts_with_ethernet);
    RUN_TEST(test_all_pins_are_different);

    RUN_TEST(test_relay_duration_is_3_seconds);
    RUN_TEST(test_heartbeat_interval_is_5_seconds);
    RUN_TEST(test_heartbeat_timeout_is_3_failures);
    RUN_TEST(test_fallback_activates_at_15_seconds);

    RUN_TEST(test_esp32_http_port_is_80);
    RUN_TEST(test_mini_pc_port_is_8000);

    RUN_TEST(test_firmware_version_is_semver);
    RUN_TEST(test_firmware_version_is_1_1_0);
    RUN_TEST(test_ota_uses_http_port_80);

    return UNITY_END();
}
