#include <pgmspace.h>
 
// ====== secrets_new.h ======
#define SECRET_NEW
 
// WiFi credentials
#define WIFI_SSID     "VM1080293"
#define WIFI_PASSWORD "Omidmuhsin2015"


// MQTT client ID
#define Client_ID "ESP32_ID_1"

 
// AWS IoT endpoint
// Copy from AWS IoT Core console
// Replace with your AWS IoT endpoint
// Example: "your-endpoint-ats.iot.your-region.amazonaws.com"
const char AWS_IOT_ENDPOINT[] = "you endpoint"; 
 
// Copy contents from AmazonRootCA1.pem here ▼  
// CA certificate
// Use triple quotes R"EOF(  )EOF" to handle multi-line strings
// Store in PROGMEM to save RAM
// Using raw string literals for multi-line strings
// Copy contents from AmazonRootCA1.pem here ▼
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----

)EOF";
 
 
// Copy contents from XXXXXXXX-certificate.pem.crt here ▼
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----


 
)KEY";
 
 
// Copy contents from  XXXXXXXX-private.pem.key here ▼
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----

 
)KEY";
