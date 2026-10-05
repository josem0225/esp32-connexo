// ============================================================
// ReleTemporizado — la apertura del relé (connexo-documentacion/fallos/07, F4)
//
// Antes: con el relé activo, una orden nueva recibía 409 y se descartaba. Si el loop
// estaba trabado, el relé se apagaba justo cuando la segunda persona iba a pasar.
// Ahora: una orden nueva ALARGA la apertura desde ese momento.
// ============================================================
#include <unity.h>
#include "ReleTemporizado.h"

void setUp() {}
void tearDown() {}

// ── positivo ──
void test_positivo_activar_enciende_y_apaga_a_los_3s() {
    ReleTemporizado rele(3000);
    TEST_ASSERT_TRUE(rele.activar(1000));          // estaba apagado: hay que encender el pin
    TEST_ASSERT_TRUE(rele.activo());
    TEST_ASSERT_FALSE(rele.debeApagar(3999));
    TEST_ASSERT_TRUE(rele.debeApagar(4000));
    TEST_ASSERT_FALSE(rele.activo());
}

void test_positivo_cuenta_las_activaciones() {
    ReleTemporizado rele(3000);
    rele.activar(0); rele.debeApagar(3000);
    rele.activar(5000);
    TEST_ASSERT_EQUAL_UINT32(2, rele.activaciones());
}

// ── negativo ──
void test_negativo_apagado_no_pide_apagar() {
    ReleTemporizado rele(3000);
    TEST_ASSERT_FALSE(rele.debeApagar(10000));
    TEST_ASSERT_FALSE(rele.activo());
}

void test_negativo_debeApagar_solo_una_vez() {
    ReleTemporizado rele(3000);
    rele.activar(0);
    TEST_ASSERT_TRUE(rele.debeApagar(3000));
    TEST_ASSERT_FALSE(rele.debeApagar(3001));      // ya se apagó: no repetir
}

// ── borde ──
void test_borde_segunda_orden_alarga_en_vez_de_rechazar() {
    ReleTemporizado rele(3000);
    rele.activar(0);
    TEST_ASSERT_FALSE(rele.activar(2000));         // ya estaba encendido: no hay que tocar el pin
    TEST_ASSERT_FALSE(rele.debeApagar(3000));      // la apertura de A ya no corta a B
    TEST_ASSERT_TRUE(rele.debeApagar(5000));       // B tiene sus 3 s completos
}

void test_borde_desborde_de_millis_cerca_de_49_dias() {
    ReleTemporizado rele(3000);
    unsigned long casiElTope = 0xFFFFFFFFUL - 1000;
    rele.activar(casiElTope);
    TEST_ASSERT_FALSE(rele.debeApagar(casiElTope + 1000));   // = 0xFFFFFFFF
    TEST_ASSERT_TRUE(rele.debeApagar(casiElTope + 3000));    // dio la vuelta: 1999
}

// ── peor caso ──
void test_peor_caso_mayerly_loop_trabado_13s() {
    // A abre en 0. El loop no corre hasta los 13 s. B pone la cara a los 9 s.
    ReleTemporizado rele(3000);
    rele.activar(0);
    rele.activar(9000);                            // B: antes 409, ahora alarga hasta 12 s
    TEST_ASSERT_FALSE(rele.debeApagar(11999));
    TEST_ASSERT_TRUE(rele.debeApagar(13000));      // el loop vuelve: B tuvo su apertura
}

void test_peor_caso_rafaga_de_ordenes_no_deja_el_rele_pegado_para_siempre() {
    ReleTemporizado rele(3000);
    for (unsigned long t = 0; t < 10000; t += 500) rele.activar(t);   // 20 órdenes seguidas
    TEST_ASSERT_TRUE(rele.debeApagar(9500 + 3000));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_positivo_activar_enciende_y_apaga_a_los_3s);
    RUN_TEST(test_positivo_cuenta_las_activaciones);
    RUN_TEST(test_negativo_apagado_no_pide_apagar);
    RUN_TEST(test_negativo_debeApagar_solo_una_vez);
    RUN_TEST(test_borde_segunda_orden_alarga_en_vez_de_rechazar);
    RUN_TEST(test_borde_desborde_de_millis_cerca_de_49_dias);
    RUN_TEST(test_peor_caso_mayerly_loop_trabado_13s);
    RUN_TEST(test_peor_caso_rafaga_de_ordenes_no_deja_el_rele_pegado_para_siempre);
    return UNITY_END();
}
