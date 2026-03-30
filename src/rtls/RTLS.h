#pragma once
#include <ArduinoJson.h>
#include "config.h"

// Initialize RTLS module
void RTLS_begin();

// Update RTLS state (call in main loop)
void RTLS_update();

// Fill JSON document with RTLS data
void RTLS_fill(JsonDocument& doc);

// Set RTLS mode ("all" or "valid") — controlled by React/Node-RED
void RTLS_setMode(const char* mode);
