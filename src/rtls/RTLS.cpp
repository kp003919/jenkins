#include "RTLS.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// =========================
// Mode Flag (React Controls This)
// =========================
bool SHOW_ALL_BEACONS = false;   // false = only valid beacons, true = all devices

// =========================
// Beacon Structure
// =========================
struct BeaconInfo {
    uint32_t id;            // 32-bit unique ID
    float rssiSmoothed;
    uint8_t battery;
    uint8_t type;
    uint32_t lastSeen;
    bool hasRssi = false;
};

// Storage
static BeaconInfo beacons[20];
static uint8_t beaconCount = 0;

// BLE Scan object
static BLEScan* pBLEScan = nullptr;

// RSSI smoothing
static const float EMA_ALPHA = 0.30f;

// Expire beacons after 2 minutes
static const uint32_t BEACON_TIMEOUT = 120000; // 2 minutes



// =========================
// Utility: Update RSSI
// =========================
void updateRSSI(BeaconInfo& b, int newRssi) {
    if (!b.hasRssi) {
        b.rssiSmoothed = newRssi;
        b.hasRssi = true;
    } else {
        b.rssiSmoothed = EMA_ALPHA * newRssi + (1.0f - EMA_ALPHA) * b.rssiSmoothed;
    }
}

// =========================
// 32-bit MAC Hash (djb2)
// =========================
uint32_t hashMac(const std::string& mac) {
    uint32_t hash = 5381;
    for (char c : mac) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}

// =========================
// UNIVERSAL MANUFACTURER PARSER
// =========================
bool parseManufacturer(BLEAdvertisedDevice& device,
                       uint32_t& id,
                       uint8_t& battery,
                       uint8_t& type)
{
    if (!device.haveManufacturerData())
        return false;

    std::string mfg = device.getManufacturerData();
    const uint8_t* raw = (const uint8_t*)mfg.data();
    size_t len = mfg.length();

    // -----------------------------------------
    // 1. Custom ESP32 / Raspberry Pi beacons
    // Format: FF FF [id] [battery] [type]
    // -----------------------------------------
    if (len >= 5 && raw[0] == 0xFF && raw[1] == 0xFF) {
        id      = raw[2];
        battery = raw[3];
        type    = raw[4];
        return true;
    }

    // -----------------------------------------
    // 2. iBeacon (Apple)
    // -----------------------------------------
    if (len >= 4 && raw[0] == 0x4C && raw[1] == 0x00 &&
        raw[2] == 0x02 && raw[3] == 0x15)
    {
        type = 1;
        id = raw[4] ^ raw[5] ^ raw[6] ^ raw[7]; // stable hash
        battery = 0;
        return true;
    }

    // -----------------------------------------
    // 3. Eddystone (UUID FEAA)
    // -----------------------------------------
    if (device.haveServiceUUID() &&
        device.getServiceUUID().equals(BLEUUID((uint16_t)0xFEAA)))
    {
        type = 2;
        id = raw[0] ^ raw[1] ^ raw[2];
        battery = 0;
        return true;
    }

    // Unknown manufacturer data → still valid
    id = hashMac(mfg);
    battery = 0;
    type = 0;
    return true;
}

// =========================
// BLE Callback
// =========================
class RTLSCallback : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device) override {

        int rssi = device.getRSSI();
        uint32_t id = 0;
        uint8_t battery = 0;
        uint8_t type = 0;

        // Try to parse manufacturer data
        bool valid = parseManufacturer(device, id, battery, type);

        // If invalid and we are in filtered mode → ignore
        if (!valid && !SHOW_ALL_BEACONS) {
            return;
        }

        // If invalid but SHOW_ALL_BEACONS = true → generate synthetic ID
        if (!valid && SHOW_ALL_BEACONS) {
            std::string mac = device.getAddress().toString();
            id = hashMac(mac);
            battery = 0;
            type = 0;
        }

        Serial.printf("MAC %s → ID %u\n",
            device.getAddress().toString().c_str(), id);

        // -----------------------------
        // Update or add beacon
        // -----------------------------
        for (uint8_t i = 0; i < beaconCount; i++) {
            if (beacons[i].id == id) {
                updateRSSI(beacons[i], rssi);
                beacons[i].battery  = battery;
                beacons[i].type     = type;
                beacons[i].lastSeen = millis();
                return;
            }
        }

        // Add new beacon
        if (beaconCount < 20) {
            beacons[beaconCount].id       = id;
            beacons[beaconCount].battery  = battery;
            beacons[beaconCount].type     = type;
            beacons[beaconCount].lastSeen = millis();
            updateRSSI(beacons[beaconCount], rssi);
            beaconCount++;
        }
    }
};

// =========================
// RTLS Initialization
// =========================
void RTLS_begin() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new RTLSCallback());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(80);
    pBLEScan->setWindow(79);
}

// =========================
// RTLS Update
// =========================
void RTLS_update() {
    static uint32_t lastScan = 0;
    if (!pBLEScan) return;

    if (millis() - lastScan >= 2000) {
        lastScan = millis();
        pBLEScan->start(1, false);
        pBLEScan->clearResults();
    }

    // Remove expired beacons
    uint32_t now = millis();
    for (uint8_t i = 0; i < beaconCount; i++) {
        if (now - beacons[i].lastSeen > BEACON_TIMEOUT) {
            beacons[i] = beacons[beaconCount - 1];
            beaconCount--;
            i--;
        }
    }
}

// =========================
// Fill JSON
// =========================
void RTLS_fill(JsonDocument& doc) {
    JsonArray arr = doc.createNestedArray("beacons");
    for (uint8_t i = 0; i < beaconCount; i++) {
        JsonObject b = arr.createNestedObject();
        b["id"]       = beacons[i].id;
        b["rssi"]     = (int)beacons[i].rssiSmoothed;
        b["battery"]  = beacons[i].battery;
        b["type"]     = beacons[i].type;
        b["lastSeen"] = beacons[i].lastSeen;
    }
}

// =========================
// Mode Setter (React Controls This)
// =========================
void RTLS_setMode(const char* mode) {
    if (strcmp(mode, "all") == 0) {
        SHOW_ALL_BEACONS = true;
    } else if (strcmp(mode, "valid") == 0) {
        SHOW_ALL_BEACONS = false;
    }
}
