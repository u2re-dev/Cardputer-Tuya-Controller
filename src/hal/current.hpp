#pragma once

//
#include <library/std.hpp>
#include <hal/core/interface.hpp>

//
std::function<void(uint32_t)> COM_HANDLER;

//
static std::atomic<uint32_t> EXCEPTION;
static std::atomic<bool> LOADING_SD;
static std::atomic<uint> CURRENT_DEVICE;
static std::atomic<bool> DEBUG_SCREEN;
static std::atomic<uint32_t> BG_COLOR;
static const uintptr_t STOP_TIMEOUT = 1000;

//
#ifdef ENABLE_TICK
static uintptr_t LAST_ACTIVE_TIME = millis();
#endif

//
std::atomic<bool> INTERRUPTED;
std::atomic<uintptr_t> LAST_TIME; //= millis();

//
std::atomic<bool> POWER_SAVING;
std::atomic<bool> OVERLAY_CHANGED;
std::atomic<bool> SHOW_CHANGED;

//
void wakeUp() {
    SHOW_CHANGED = false;
#ifdef ENABLE_CTL
    digitalWrite(PIN_POWER_ON, HIGH);
    digitalWrite(PIN_LCD_BL, HIGH);
    setCpuFrequencyMhz(240);
#endif
#ifdef ENABLE_TICK
    LAST_ACTIVE_TIME = millis();
#endif
    POWER_SAVING = false;
    SHOW_CHANGED = true;
}

//
void powerSave() {
#ifdef ENABLE_CTL
    SHOW_CHANGED = false;
    digitalWrite(PIN_POWER_ON, LOW);
    digitalWrite(PIN_LCD_BL, LOW);
    setCpuFrequencyMhz(80);
    POWER_SAVING = true;
#endif
}

//
void initState() {
    SHOW_CHANGED = false;
    EXCEPTION = -1;
    OVERLAY_CHANGED = false;
    POWER_SAVING = true;
    BG_COLOR = 0x0000;
    CURRENT_DEVICE = 0;
    DEBUG_SCREEN = true;
#ifdef ENABLE_TICK
    LAST_TIME = millis();
#endif
    INTERRUPTED = false;
    LOADING_SD = false;
    SHOW_CHANGED = true;
#ifdef ENABLE_CTL
    setCpuFrequencyMhz(80);
#endif
}

//
#ifdef ENABLE_ARDUINO
struct OVERLAY {
    _String_<> _LEFT_;
    _String_<> _RIGHT_;
} overlay;
#endif

//
void switchScreen(bool dbg, uint dvID) {
    if (DEBUG_SCREEN != dbg || CURRENT_DEVICE != dvID) {
        SHOW_CHANGED = false;
        DEBUG_SCREEN = dbg;
        CURRENT_DEVICE = std::max(std::min(dvID, 1u), 0u);
        SHOW_CHANGED = true;
    }
}
