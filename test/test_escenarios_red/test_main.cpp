// ============================================================
// Escenarios de red, de suave a letal (connexo-documentacion/fallos/08).
//
// La BusquedaDelBox REAL del firmware contra una red simulada con reloj virtual.
// Reproduce lo que hace tareaRed (main.cpp) cada 100 ms:
//   - si debeBuscar → consulta mDNS (bloquea ~3 s en el ESP32 real);
//   - cada 5 s → latido al destino (timeout de conexión 800 ms si no contesta).
// El latido solo funciona si el box está prendido Y el destino es su IP actual.
// ============================================================
#include <unity.h>
#include <string.h>
#include <functional>
#include "BusquedaDelBox.h"

void setUp() {}
void tearDown() {}

static const unsigned long SEG = 1000UL;
static const unsigned long MIN = 60UL * SEG;

struct Red {
    // Estado del box en el instante t: "" = apagado / no anunciado.
    std::function<const char*(unsigned long)> ipDelBox;
    // IP que devuelve el mDNS (por defecto, la del box). Permite simular una caché vieja.
    std::function<const char*(unsigned long)> mdnsResponde;
};

struct Resultado {
    unsigned long busquedas = 0;
    unsigned long latidosOk = 0;
    unsigned long latidosFallidos = 0;
    long primerOkDesde = -1;             // primer latido OK a partir de `desde`
    bool usoIpInventada = false;         // latió a una IP que el mDNS nunca dio
    unsigned long maxBusquedasEnUnMinuto = 0;
};

static Resultado simular(Red red, unsigned long duracion, unsigned long desde = 0) {
    BusquedaDelBox b(60000, 3);
    Resultado r;
    unsigned long t = 0, ultimoLatido = 0;
    bool primerLatido = true;
    unsigned long ventanaInicio = 0, enVentana = 0;
    char ipsDadas[16][40]; int nDadas = 0;

    while (t < duracion) {
        if (b.debeBuscar(t)) {
            r.busquedas++;
            if (t - ventanaInicio >= MIN) { ventanaInicio = t; enVentana = 0; }
            enVentana++;
            if (enVentana > r.maxBusquedasEnUnMinuto) r.maxBusquedasEnUnMinuto = enVentana;
            b.buscado(t);
            const char* ip = red.mdnsResponde ? red.mdnsResponde(t) : red.ipDelBox(t);
            t += 3 * SEG;                                  // MDNS.queryService bloquea ~3 s
            if (ip && ip[0]) {
                b.encontrado(ip, 8000);
                if (nDadas < 16) { strncpy(ipsDadas[nDadas], ip, 39); ipsDadas[nDadas][39] = 0; nDadas++; }
            }
        }
        if (b.tieneDestino() && (primerLatido || t - ultimoLatido >= 5 * SEG)) {
            primerLatido = false;
            ultimoLatido = t;
            bool dada = false;
            for (int i = 0; i < nDadas; i++) if (strcmp(ipsDadas[i], b.ip()) == 0) dada = true;
            if (!dada) r.usoIpInventada = true;
            const char* ipBox = red.ipDelBox(t);
            if (ipBox && ipBox[0] && strcmp(ipBox, b.ip()) == 0) {
                b.latidoOk(); r.latidosOk++;
                if (r.primerOkDesde < 0 && t >= desde) r.primerOkDesde = (long)(t - desde);
            } else {
                b.latidoFallido(t); r.latidosFallidos++;
                t += 800;                                  // timeout de conexión
            }
        }
        t += 100;                                          // vTaskDelay(100)
    }
    return r;
}

static const char* siempre(const char* ip) { return ip; }

// ── N1 suave: todo prendido desde el inicio ──
void test_N1_todo_prendido_late_enseguida() {
    Red red{[](unsigned long) { return "192.168.1.18"; }, nullptr};
    Resultado r = simular(red, 5 * MIN);
    TEST_ASSERT_TRUE(r.primerOkDesde >= 0 && r.primerOkDesde <= 4 * (long)SEG);
    TEST_ASSERT_EQUAL_UINT32(0, r.latidosFallidos);
    TEST_ASSERT_EQUAL_UINT32(1, r.busquedas);
}

// ── N2: el ESP32 prende antes que el box (el box tarda 2 min) ──
void test_N2_esp32_antes_que_el_box() {
    Red red{[](unsigned long t) { return t < 2 * MIN ? "" : "192.168.1.18"; }, nullptr};
    Resultado r = simular(red, 6 * MIN, 2 * MIN);
    TEST_ASSERT_TRUE_MESSAGE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG,
                             "debe latir <= ~60 s después de que el box aparece");
    TEST_ASSERT_FALSE(r.usoIpInventada);
}

// ── N3: el box se reinicia 1 min (un Actualizar) ──
void test_N3_reinicio_de_un_minuto() {
    Red red{[](unsigned long t) { return (t >= 5 * MIN && t < 6 * MIN) ? "" : "192.168.1.18"; }, nullptr};
    Resultado r = simular(red, 10 * MIN, 6 * MIN);
    TEST_ASSERT_TRUE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG);
}

// ── N4: el corte del 23-sep, 16 min ──
void test_N4_corte_de_16_minutos_sin_ip_inventada() {
    Red red{[](unsigned long t) { return (t >= 10 * MIN && t < 26 * MIN) ? "" : "192.168.1.18"; }, nullptr};
    Resultado r = simular(red, 40 * MIN, 26 * MIN);
    TEST_ASSERT_TRUE_MESSAGE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG,
                             "el firmware 1.1.0 se quedaba 17 días pegado aquí");
    TEST_ASSERT_FALSE(r.usoIpInventada);
}

// ── N5: el box vuelve con OTRA IP ──
void test_N5_el_box_vuelve_con_otra_ip() {
    Red red{[](unsigned long t) -> const char* {
        if (t < 5 * MIN) return "192.168.1.18";
        if (t < 7 * MIN) return "";
        return "192.168.1.42";
    }, nullptr};
    Resultado r = simular(red, 15 * MIN, 7 * MIN);
    TEST_ASSERT_TRUE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG);
}

// ── N6: el anuncio del box falla al arrancar y después funciona ──
void test_N6_anuncio_tardio_del_box() {
    // El box está prendido (latidos funcionarían) pero el mDNS no lo anuncia los primeros 3 min.
    Red red{[](unsigned long) { return "192.168.1.18"; },
            [](unsigned long t) { return t < 3 * MIN ? "" : "192.168.1.18"; }};
    Resultado r = simular(red, 8 * MIN, 3 * MIN);
    TEST_ASSERT_TRUE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG);
    TEST_ASSERT_EQUAL_UINT32(0, r.latidosFallidos);         // sin destino no se late a ciegas
}

// ── N8 💀: el box parpadea cada 30 s durante 10 min ──
void test_N8_box_que_parpadea_y_luego_se_estabiliza() {
    Red red{[](unsigned long t) -> const char* {
        if (t < 10 * MIN) return ((t / (30 * SEG)) % 2) ? "" : "192.168.1.18";
        return "192.168.1.18";
    }, nullptr};
    Resultado r = simular(red, 20 * MIN, 10 * MIN);
    TEST_ASSERT_TRUE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG);
    TEST_ASSERT_TRUE(r.maxBusquedasEnUnMinuto <= 2);
}

// ── N9 💀: el mDNS del box no funciona nunca ──
void test_N9_mdns_nunca_responde_no_martilla_ni_inventa() {
    Red red{[](unsigned long) { return "192.168.1.18"; }, [](unsigned long) { return ""; }};
    Resultado r = simular(red, 60 * MIN);
    TEST_ASSERT_EQUAL_UINT32(0, r.latidosOk);
    TEST_ASSERT_EQUAL_UINT32(0, r.latidosFallidos);         // sin destino, no late a nadie
    TEST_ASSERT_FALSE(r.usoIpInventada);
    TEST_ASSERT_TRUE(r.busquedas <= 61);                     // ≈ 1 por minuto
    TEST_ASSERT_TRUE(r.maxBusquedasEnUnMinuto <= 1);
}

// ── N10 💀: corte de 6 h (la noche entera) ──
void test_N10_corte_de_seis_horas() {
    Red red{[](unsigned long t) { return (t >= 10 * MIN && t < 370 * MIN) ? "" : "192.168.1.18"; }, nullptr};
    Resultado r = simular(red, 400 * MIN, 370 * MIN);
    TEST_ASSERT_TRUE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG);
    TEST_ASSERT_TRUE_MESSAGE(r.maxBusquedasEnUnMinuto <= 2, "no puede martillar la red toda la noche");
    TEST_ASSERT_TRUE(r.busquedas <= 370);
}

// ── N11 💀: el mDNS devuelve una IP vieja del box (caché) durante 5 min ──
void test_N11_mdns_con_ip_vieja_se_corrige_solo() {
    Red red{[](unsigned long t) { return t < 5 * MIN ? "192.168.1.18" : "192.168.1.42"; },
            [](unsigned long t) { return t < 10 * MIN ? "192.168.1.18" : "192.168.1.42"; }};
    Resultado r = simular(red, 20 * MIN, 10 * MIN);
    TEST_ASSERT_TRUE_MESSAGE(r.primerOkDesde >= 0 && r.primerOkDesde <= 65 * (long)SEG,
                             "cuando el mDNS se corrige, el ESP32 lo toma en el siguiente minuto");
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_N1_todo_prendido_late_enseguida);
    RUN_TEST(test_N2_esp32_antes_que_el_box);
    RUN_TEST(test_N3_reinicio_de_un_minuto);
    RUN_TEST(test_N4_corte_de_16_minutos_sin_ip_inventada);
    RUN_TEST(test_N5_el_box_vuelve_con_otra_ip);
    RUN_TEST(test_N6_anuncio_tardio_del_box);
    RUN_TEST(test_N8_box_que_parpadea_y_luego_se_estabiliza);
    RUN_TEST(test_N9_mdns_nunca_responde_no_martilla_ni_inventa);
    RUN_TEST(test_N10_corte_de_seis_horas);
    RUN_TEST(test_N11_mdns_con_ip_vieja_se_corrige_solo);
    return UNITY_END();
}
