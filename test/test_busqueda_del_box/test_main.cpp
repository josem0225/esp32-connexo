// ============================================================
// BusquedaDelBox — a quién le late el ESP32 (connexo-documentacion/fallos/07, F4)
//
// Antes: buscaba al box UNA vez al entrar en fallback; si fallaba (el box estaba
// reiniciando) caía en la IP quemada 192.168.2.19 —la de la casa de Jose— y se quedaba
// ahí para siempre, trabándose en cada latido.
// Ahora: sin IP quemada (sin box conocido no se late), y se busca cada 60 s mientras
// no haya box o esté en fallback.
// ============================================================
#include <unity.h>
#include <string.h>
#include "BusquedaDelBox.h"

void setUp() {}
void tearDown() {}

// ── positivo ──
void test_positivo_al_arrancar_hay_que_buscar_y_no_hay_destino() {
    BusquedaDelBox b(60000, 3);
    TEST_ASSERT_TRUE(b.debeBuscar(0));
    TEST_ASSERT_FALSE(b.tieneDestino());
}

void test_positivo_encontrado_da_destino_y_deja_de_buscar() {
    BusquedaDelBox b(60000, 3);
    b.buscado(0);
    b.encontrado("192.168.1.18", 8000);
    TEST_ASSERT_TRUE(b.tieneDestino());
    TEST_ASSERT_EQUAL_STRING("192.168.1.18", b.ip());
    TEST_ASSERT_EQUAL_INT(8000, b.puerto());
    TEST_ASSERT_FALSE(b.debeBuscar(120000));
}

// ── negativo ──
void test_negativo_sin_destino_no_hay_ip_quemada() {
    BusquedaDelBox b(60000, 3);
    TEST_ASSERT_EQUAL_STRING("", b.ip());
}

void test_negativo_dos_fallos_no_son_fallback() {
    BusquedaDelBox b(60000, 3);
    b.encontrado("192.168.1.18", 8000);
    b.latidoFallido(1000); b.latidoFallido(2000);
    TEST_ASSERT_FALSE(b.enFallback());
    TEST_ASSERT_FALSE(b.debeBuscar(2000));
}

// ── borde ──
void test_borde_tres_fallos_entran_en_fallback_y_busca_ya() {
    BusquedaDelBox b(60000, 3);
    b.encontrado("192.168.1.18", 8000);
    b.latidoFallido(1000); b.latidoFallido(2000); b.latidoFallido(3000);
    TEST_ASSERT_TRUE(b.enFallback());
    TEST_ASSERT_TRUE(b.debeBuscar(3000));
}

void test_borde_un_latido_bueno_saca_del_fallback() {
    BusquedaDelBox b(60000, 3);
    b.encontrado("192.168.1.18", 8000);
    for (int i = 0; i < 5; i++) b.latidoFallido(i * 1000);
    b.latidoOk();
    TEST_ASSERT_FALSE(b.enFallback());
    TEST_ASSERT_EQUAL_INT(0, b.fallos());
}

// ── peor caso ──
void test_peor_caso_el_box_reinicia_y_la_primera_busqueda_falla_pero_se_reintenta() {
    // Lo que dejó al ESP32 de Suárez León pegado 17 días: buscó UNA vez, con el box caído.
    BusquedaDelBox b(60000, 3);
    b.encontrado("192.168.1.18", 8000);
    for (int i = 0; i < 3; i++) b.latidoFallido(i * 5000);
    TEST_ASSERT_TRUE(b.debeBuscar(10000));
    b.buscado(10000);                                  // no lo encontró: el box arrancando
    TEST_ASSERT_FALSE(b.debeBuscar(30000));            // no martillar
    TEST_ASSERT_TRUE(b.debeBuscar(70000));             // pero vuelve a buscar al minuto
    b.encontrado("192.168.1.18", 8000);
    b.latidoOk();
    TEST_ASSERT_FALSE(b.enFallback());
}

void test_peor_caso_el_box_cambia_de_ip() {
    BusquedaDelBox b(60000, 3);
    b.encontrado("192.168.1.18", 8000);
    for (int i = 0; i < 3; i++) b.latidoFallido(i * 5000);
    b.buscado(15000);
    b.encontrado("192.168.1.42", 8000);
    TEST_ASSERT_EQUAL_STRING("192.168.1.42", b.ip());
}

void test_peor_caso_ip_demasiado_larga_no_desborda() {
    BusquedaDelBox b(60000, 3);
    b.encontrado("123456789012345678901234567890123456789012345678901234567890", 8000);
    TEST_ASSERT_TRUE(strlen(b.ip()) < BusquedaDelBox::LARGO_IP);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_positivo_al_arrancar_hay_que_buscar_y_no_hay_destino);
    RUN_TEST(test_positivo_encontrado_da_destino_y_deja_de_buscar);
    RUN_TEST(test_negativo_sin_destino_no_hay_ip_quemada);
    RUN_TEST(test_negativo_dos_fallos_no_son_fallback);
    RUN_TEST(test_borde_tres_fallos_entran_en_fallback_y_busca_ya);
    RUN_TEST(test_borde_un_latido_bueno_saca_del_fallback);
    RUN_TEST(test_peor_caso_el_box_reinicia_y_la_primera_busqueda_falla_pero_se_reintenta);
    RUN_TEST(test_peor_caso_el_box_cambia_de_ip);
    RUN_TEST(test_peor_caso_ip_demasiado_larga_no_desborda);
    return UNITY_END();
}
