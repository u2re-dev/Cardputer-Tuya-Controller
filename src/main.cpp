//#include <Arduino.h>

//
#define VERY_LARGE_STRING_LENGTH 8000

//
#include "./core/output/tft_display.hpp"
#include "./handler/device.hpp"
#include "./handler/command.hpp"
#include "./handler/fs.hpp"

//
void loopTask(void *pvParameters)
{
    setCpuFrequencyMhz(80);

    //
    initState();
    tft::initDisplay();

    //
    Serial.setDebugOutput(true);
    Serial.begin(115200);

    //
    nv::storage.begin("nvs", false);

    //
    rtc::initRTC();
    keypad::initInput(COMHandler);

    //
    if (!fs::sd::loadConfig(FSHandler)) {
        if (!fs::internal::loadConfig(FSHandler)) {
            _STOP_EXCEPTION_();
        }
    }

    //
    wifi::initWiFi();
    while (!wifi::WiFiConnected())
    { keypad::handleInput(); delay(POWER_SAVING.load() ? 100 : 1); }

    //
    http::initServer(device);

    //
    Serial.println("Setup is done...");
    wakeUp();

    //
    while (!INTERRUPTED.load()) {
        //
        if ((millis() - LAST_ACTIVE_TIME) > 10000) {
            powerSave();
        }

        //
        {
            switchScreen((!wifi::CONNECTED.load() || LOADING_SD), CURRENT_DEVICE);

            //
            keypad::handleInput();
            wifi::handleWiFi();

            // 
            if (wifi::WiFiConnected()) { rtc::timeClient.update(); }
            rtc::_syncTimeFn_();

            //
            handleDevices();
        }

        //
        delay(POWER_SAVING.load() ? 100 : 1);
    }

    //
    _STOP_EXCEPTION_();
    while (!(POWER_SAVING.load() || (millis() - LAST_TIME.load()) >= STOP_TIMEOUT)) {
        delay(POWER_SAVING.load() ? 100 : 1);
    }

    //
#ifdef ESP32
    ESP.restart();
#else
    ESP.reset();
#endif
}

//
#if CONFIG_FREERTOS_UNICORE
void yieldIfNecessary(void){
    static uint64_t lastYield = 0;
    uint64_t now = millis();
    if((now - lastYield) > 2000) {
        lastYield = now;
        vTaskDelay(5); //delay 1 RTOS tick
    }
}
#endif

//
#if !CONFIG_AUTOSTART_ARDUINO
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

//
#ifndef ARDUINO_LOOP_STACK_SIZE
#ifndef CONFIG_ARDUINO_LOOP_STACK_SIZE
#define ARDUINO_LOOP_STACK_SIZE 8192
#else
#define ARDUINO_LOOP_STACK_SIZE CONFIG_ARDUINO_LOOP_STACK_SIZE
#endif
#endif

//
TaskHandle_t loopTaskHandle = NULL;
bool loopTaskWDTEnabled;

//
__attribute__((weak)) size_t getArduinoLoopTaskStackSize(void) { return ARDUINO_LOOP_STACK_SIZE; }
__attribute__((weak)) bool shouldPrintChipDebugReport(void) { return false; }

//
extern "C" void app_main()
{
#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
    Serial.begin();
#endif
#if ARDUINO_USB_MSC_ON_BOOT && !ARDUINO_USB_MODE
    MSC_Update.begin();
#endif
#if ARDUINO_USB_DFU_ON_BOOT && !ARDUINO_USB_MODE
    USB.enableDFU();
#endif
#if ARDUINO_USB_ON_BOOT && !ARDUINO_USB_MODE
    USB.begin();
#endif
    loopTaskWDTEnabled = false;
    initArduino();
    xTaskCreateUniversal(loopTask, "loopTask", getArduinoLoopTaskStackSize(), NULL, 1, &loopTaskHandle, ARDUINO_RUNNING_CORE);
}

#else

// unsupported...
void setup() {};
void loop() {};

#endif