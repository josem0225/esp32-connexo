// ============================================================
// MedidorDeLoop — cuánto se traba el loop() (connexo-documentacion/fallos/07, F4)
// Va en /status para que el box lo lea y lo deje en su log: el bug de Suárez León
// estuvo 17 días sin que nada lo mostrara.
// ============================================================
#include <unity.h>
#include "MedidorDeLoop.h"

void setUp() {}
void tearDown() {}

void test_positivo_loop_sano_mide_poco() {
    MedidorDeLoop m;
    for (unsigned long t = 0; t <= 1000; t += 10) m.vuelta(t);
    TEST_ASSERT_EQUAL_UINT32(10, m.maximoMs());
}

void test_negativo_la_primera_vuelta_no_cuenta() {
    MedidorDeLoop m;
    m.vuelta(50000);                    // desde el arranque: no es una vuelta trabada
    TEST_ASSERT_EQUAL_UINT32(0, m.maximoMs());
}

void test_borde_desborde_de_millis() {
    MedidorDeLoop m;
    m.vuelta(0xFFFFFFFFUL - 5);
    m.vuelta(4);                        // 10 ms después, dando la vuelta
    TEST_ASSERT_EQUAL_UINT32(10, m.maximoMs());
}

void test_peor_caso_un_bloqueo_de_13s_queda_registrado() {
    MedidorDeLoop m;
    m.vuelta(0); m.vuelta(10); m.vuelta(13010); m.vuelta(13020);
    TEST_ASSERT_EQUAL_UINT32(13000, m.maximoMs());
    TEST_ASSERT_EQUAL_UINT32(13010, m.cuandoMs());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_positivo_loop_sano_mide_poco);
    RUN_TEST(test_negativo_la_primera_vuelta_no_cuenta);
    RUN_TEST(test_borde_desborde_de_millis);
    RUN_TEST(test_peor_caso_un_bloqueo_de_13s_queda_registrado);
    return UNITY_END();
}
