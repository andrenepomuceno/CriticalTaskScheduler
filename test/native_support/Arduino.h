// Minimal Arduino.h stub for native (host) unit tests.
// Provides only what TaskScheduler.cpp/h need.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// ---- millis() -----------------------------------------------------------
extern "C" unsigned long millis();

// ---- Print interface (for Scheduler::printStats) ------------------------
class Print
{
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size)
    {
        size_t n = 0;
        while (size--) n += write(*buffer++);
        return n;
    }
    size_t print(const char *s)            { return write(reinterpret_cast<const uint8_t *>(s), s ? std::strlen(s) : 0); }
    size_t print(unsigned long v)          { char b[32]; int n = std::snprintf(b, sizeof(b), "%lu", v); return write(reinterpret_cast<const uint8_t *>(b), n); }
    size_t print(int v)                    { char b[32]; int n = std::snprintf(b, sizeof(b), "%d", v); return write(reinterpret_cast<const uint8_t *>(b), n); }
    size_t print(float v, int decimals = 2){ char b[32]; int n = std::snprintf(b, sizeof(b), "%.*f", decimals, v); return write(reinterpret_cast<const uint8_t *>(b), n); }
    size_t println(const char *s)          { size_t n = print(s); n += write('\n'); return n; }
    size_t println(unsigned long v)        { size_t n = print(v); n += write('\n'); return n; }
    size_t println(int v)                  { size_t n = print(v); n += write('\n'); return n; }
    size_t println(float v, int decimals=2){ size_t n = print(v, decimals); n += write('\n'); return n; }
};

// ---- F() macro / String stubs (unused by core, but harmless) ------------
#define F(s) (s)
