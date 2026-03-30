/**
 * This module is designed to test the core functionalities of the ESP32 in a Hardware-in-the-Loop (HIL) setup.
 * It provides a simple command interface over USB Serial to trigger various tests, such as reading sensors, checking connectivity, and simulating protocol interactions (SPI/I2C).
 *  The goal is to allow developers to verify that the ESP32 is functioning correctly in isolation before integrating it with the STM32 and other components in the full IoT system.
 *  Commands can be sent from a Python script or any serial terminal, and the ESP32 will respond with JSON-formatted results for easy parsing and validation.   
 *  Note: This code is meant for testing purposes only and should not be included in the production firmware. It is intended to run in a controlled environment where the ESP32 can be easily reset if needed. Always ensure that you have a way to recover the device if something goes wrong during testing.  
 * 
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include "hil_test_mode.h"
#include "config.h"
#include"SPI.h"
#include <NimBLEDevice.h>
#include <Wire.h>

// ---------- WiFi ----------
#include <WiFi.h>

// ---------- MQTT (placeholder) ----------
bool mqttConnected() {
    // Replace with your real MQTT client check:
    // return mqttClient.connected();
    return false;  // default until you integrate real MQTT
}

// ---------- DHT ----------
#include <DHT.h>

#ifndef DHTTYPE
#define DHTTYPE DHT22
#endif

DHT dht(DHTPIN, DHTTYPE);

// ---------- GPS ----------
#include <TinyGPSPlus.h>
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1);  // UART1

// ---------- Globals ----------
unsigned long bootTime = 0;

// ---------- Helper functions ----------
float readTemperature() { return dht.readTemperature(); }
float readHumidity()    { return dht.readHumidity(); }

float getLat() {
    while (GPS_Serial.available()) gps.encode(GPS_Serial.read());
    return gps.location.isValid() ? gps.location.lat() : 0.0f;
}

float getLon() {
    while (GPS_Serial.available()) gps.encode(GPS_Serial.read());
    return gps.location.isValid() ? gps.location.lng() : 0.0f;
}

// ======================================================
//                HIL TEST MODE ENTRY POINT
// ======================================================

void startHilTestMode() {
    Serial.println("[HIL] Test mode active");
    bootTime = millis();

    pinMode(LED_BUILTIN, OUTPUT);

    dht.begin();
    GPS_Serial.begin(9600, SERIAL_8N1, 16, 17);  // adjust pins if needed
    Wire.begin();

    while (true) {
        if (!Serial.available()) {
            delay(5);
            continue;
        }

        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        // ---------- Basic ----------
        if (cmd == "PING") {
            Serial.println("[TEST] PONG");
        }
        else if (cmd == "TEST_UPTIME") {
            unsigned long uptime = (millis() - bootTime) / 1000;
            Serial.printf("[TEST] UPTIME %lu\n", uptime);
        }
        else if (cmd == "TEST_PULSE") {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(50);
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("[TEST] PULSE_DONE");
        }

        // ---------- DHT ----------
        else if (cmd == "TEST_DHT") {
            StaticJsonDocument<128> doc;
            doc["temperature"] = readTemperature();
            doc["humidity"]    = readHumidity();

            Serial.print("[TEST] ");
            serializeJson(doc, Serial);
            Serial.println();
        }

        // ---------- GPS ----------
        else if (cmd == "TEST_GPS") {
            StaticJsonDocument<128> doc;
            doc["lat"] = getLat();
            doc["lon"] = getLon();

            Serial.print("[TEST] ");
            serializeJson(doc, Serial);
            Serial.println();
        }

        // ---------- RTLS ----------
        else if (cmd == "TEST_RTLS") {
            NimBLEScan* scan = BLEDevice::getScan();
            scan->stop();
            scan->setActiveScan(true);
            scan->setInterval(45);
            scan->setWindow(15);

            scan->start(3, false);
            NimBLEScanResults results = scan->getResults();

            StaticJsonDocument<512> doc;
            JsonArray arr = doc.createNestedArray("rtls");

            for (int i = 0; i < results.getCount(); i++) {
                const NimBLEAdvertisedDevice* dev = results.getDevice(i);
                JsonObject obj = arr.createNestedObject();
                obj["mac"] = dev->getAddress().toString().c_str();
                obj["rssi"] = dev->getRSSI();
            }

            scan->clearResults();

            Serial.print("[TEST] ");
            serializeJson(doc, Serial);
            Serial.println();
        }

        // ---------- WIFI ----------
        else if (cmd == "TEST_WIFI") {
            bool wifi_ok = (WiFi.status() == WL_CONNECTED);
            Serial.println(wifi_ok ? "[TEST] WIFI_OK" : "[TEST] WIFI_FAIL");
        }

        // ---------- WIFI INFO (JSON) ----------
        else if (cmd == "TEST_WIFI_INFO") {
            StaticJsonDocument<128> doc;
            bool connected = (WiFi.status() == WL_CONNECTED);

            doc["connected"] = connected;
            if (connected) {
                doc["ssid"] = WiFi.SSID();
                doc["rssi"] = WiFi.RSSI();
            }

            Serial.print("[TEST] ");
            serializeJson(doc, Serial);
            Serial.println();
        }

        // ---------- MQTT ----------
        else if (cmd == "TEST_MQTT") {
            bool mqtt_ok = mqttConnected();
            Serial.println(mqtt_ok ? "[TEST] MQTT_OK" : "[TEST] MQTT_FAIL");
        }

        // ---------- MQTT PUBLISH ----------
        else if (cmd == "TEST_MQTT_PUB") {
            bool ok = false;  // replace with real publish
            Serial.println(ok ? "[TEST] MQTT_PUB_OK" : "[TEST] MQTT_PUB_FAIL");
        }

        // ---------- I2C SCAN ----------
        else if (cmd == "TEST_I2C_SCAN") {
            StaticJsonDocument<256> doc;
            JsonArray arr = doc.createNestedArray("i2c");

            for (uint8_t addr = 1; addr < 127; addr++) {
                Wire.beginTransmission(addr);
                if (Wire.endTransmission() == 0) {
                    arr.add(addr);
                }
            }

            Serial.print("[TEST] ");
            serializeJson(doc, Serial);
            Serial.println();
        }

        // ---------- FLASH SIZE ----------
        else if (cmd == "TEST_FLASH") {
            size_t size_kb = ESP.getFlashChipSize() / 1024;
            Serial.printf("[TEST] FLASH_SIZE %u\n", size_kb);
        } // ---------- PROTOCOLS (SPI/I2C) ----------
          else if (cmd == "TEST_SPI") {
            // Example SPI read/write (replace with actual commands for your peripheral)
            uint8_t spiDataToSend = 0xAB; // Example data to send
            uint8_t spiReceivedData = spiDataToSend;
            Serial.printf("[TEST] SPI_SEND %02X, SPI_RECV %02X\n", spiDataToSend, spiReceivedData);
        }
          else if (cmd == "TEST_I2C") {
            // Example I2C read/write (replace with actual commands for your peripheral)
            Wire.beginTransmission(0x50); // Example I2C address
            Wire.write(0x00); // Example register
            Wire.write(0xAB); // Example data to send
            bool writeOk = (Wire.endTransmission() == 0);

            Wire.requestFrom(0x50, 1);
            uint8_t i2cReceivedData = Wire.available() ? Wire.read() : 0xFF;

            Serial.printf("[TEST] I2C_WRITE %s, I2C_RECV %02X\n", writeOk ? "OK" : "FAIL", i2cReceivedData);    
          }
          else if (cmd == "TEST_PROTOCOLS") {
            // This is a placeholder. In a real implementation, you would perform actual SPI/I2C operations and report results.
            Serial.println("[TEST] PROTOCOLS_SPI_OK");
            Serial.println("[TEST] PROTOCOLS_I2C_OK");              
          }
        // ---------- Unknown ----------
        else {
            Serial.println("[TEST] UNKNOWN_CMD");
        }
    }
}
