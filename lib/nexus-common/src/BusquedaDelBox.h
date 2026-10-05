#pragma once

// ============================================================
// BusquedaDelBox — a qué box le late el ESP32 y cuándo volver a buscarlo.
//
// Por qué existe (2026-10-05, connexo-documentacion/fallos/07): el firmware buscaba al
// box UNA vez al entrar en fallback; si en ese momento el box estaba reiniciando, caía
// en la IP quemada 192.168.2.19 (la de la casa de Jose) y se quedaba ahí para siempre.
// Ahora:
//   - sin IP quemada: sin box conocido no se late;
//   - se busca al arrancar y cada `reintentoMs` mientras no haya box o esté en fallback.
// Sin Arduino: se prueba con `pio test -e native`.
// ============================================================

#include <string.h>

class BusquedaDelBox {
public:
    static const int LARGO_IP = 40;

    BusquedaDelBox(unsigned long reintentoMs, int fallosParaFallback)
        : _reintento(reintentoMs), _fallosParaFallback(fallosParaFallback) {
        _ip[0] = '\0';
    }

    bool debeBuscar(unsigned long ahoraMs) const {
        if (tieneDestino() && !enFallback()) return false;
        if (!_buscoAlgunaVez) return true;
        return (ahoraMs - _ultimaBusqueda) >= _reintento;
    }

    void buscado(unsigned long ahoraMs) {
        _buscoAlgunaVez = true;
        _ultimaBusqueda = ahoraMs;
    }

    void encontrado(const char* ip, int puerto) {
        strncpy(_ip, ip, LARGO_IP - 1);
        _ip[LARGO_IP - 1] = '\0';
        _puerto = puerto;
    }

    void latidoOk() { _fallos = 0; }
    void latidoFallido(unsigned long /*ahoraMs*/) { _fallos++; }

    bool tieneDestino() const { return _ip[0] != '\0'; }
    bool enFallback() const { return _fallos >= _fallosParaFallback; }
    const char* ip() const { return _ip; }
    int puerto() const { return _puerto; }
    int fallos() const { return _fallos; }

private:
    unsigned long _reintento;
    int _fallosParaFallback;
    char _ip[LARGO_IP];
    int _puerto = 0;
    int _fallos = 0;
    bool _buscoAlgunaVez = false;
    unsigned long _ultimaBusqueda = 0;
};
