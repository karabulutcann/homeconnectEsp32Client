#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>

// For CA Certificate Bundle support (recommended approach)
// Uncomment the line below if you have the CA bundle available
// #include "cert.h"

// Alternative: Use built-in CA bundle (if available in your ESP32 setup)
// extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");

// SSL Configuration options
const bool SKIP_SSL_VERIFICATION = true;  // Set to false for production with proper certificates
const bool USE_CA_BUNDLE = false;        // Set to true if you have CA bundle configured

WiFiMulti wifiMulti;
WebSocketsClient webSocket;
WiFiClientSecure client;

const char *WIFI_PASSWORD = "sXhbEFWn4XNj";
const char *WIFI_NAME = "FiberHGW_HUQV5N";
const char *AUTH_TOKEN = "Bearer test";
const char *API_URL = "192.168.1.111";  // For SSL, use domain name instead of IP
const int API_PORT = 443;               // Standard HTTPS/WSS port
const int API_PORT_INSECURE = 3001;     // Fallback non-SSL port
const char *API_URI = "/socket";

// SSL Configuration
bool USE_SSL = false;  // Set to true to enable SSL, false for development

// Wake-on-LAN configuration
const char *PC_MAC_ADDRESS = "2C-F0-5D-6D-44-26"; // Replace with your PC's MAC address
const char *BROADCAST_IP = "192.168.1.255";      // Replace with your network's broadcast IP
const int WOL_PORT = 9;                          // Standard WOL port

WiFiUDP udp;



// Command enumeration for switch statement
enum Command {
    CMD_WAKE_PC,
    CMD_PING,
    CMD_UNKNOWN
};

// Function to convert string command to enum
Command getCommand(String cmd) {
    if (cmd == "wake_pc") return CMD_WAKE_PC;
    if (cmd == "ping") return CMD_PING;
    return CMD_UNKNOWN;
}

// Function to send Wake-on-LAN magic packet
void sendWakeOnLAN() {
    Serial.println("[WOL] Sending Wake-on-LAN packet...");
    
    // Parse MAC address from string format (XX:XX:XX:XX:XX:XX)
    uint8_t mac[6];
    if (sscanf(PC_MAC_ADDRESS, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        Serial.println("[WOL] Error: Invalid MAC address format!");
        return;
    }
    
    // Create magic packet (6 bytes of 0xFF followed by 16 repetitions of MAC address)
    uint8_t magicPacket[102];
    
    // Fill first 6 bytes with 0xFF
    for (int i = 0; i < 6; i++) {
        magicPacket[i] = 0xFF;
    }
    
    // Fill remaining 96 bytes with 16 repetitions of MAC address
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 6; j++) {
            magicPacket[6 + i * 6 + j] = mac[j];
        }
    }
    
    // Send UDP packet
    udp.begin(WOL_PORT);
    udp.beginPacket(BROADCAST_IP, WOL_PORT);
    udp.write(magicPacket, sizeof(magicPacket));
    
    if (udp.endPacket()) {
        Serial.printf("[WOL] Magic packet sent successfully to %s (MAC: %s)\n", BROADCAST_IP, PC_MAC_ADDRESS);
    } else {
        Serial.println("[WOL] Failed to send magic packet!");
    }
    
    udp.stop();
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);

            // send message to server when Connected
            webSocket.sendTXT("Connected");
            break;
        case WStype_TEXT:
            {
                Serial.printf("[WSc] get text: %s\n", payload);

                String cmdStr = String((char*)payload);
                Command cmd = getCommand(cmdStr);

                switch(cmd) {
                    case CMD_WAKE_PC:
                        Serial.println("[WSc] ===== WAKE PC COMMAND RECEIVED =====");
                        Serial.println("[WSc] Processing wake_pc command...");
                        sendWakeOnLAN();
                        Serial.println("[WSc] Sending confirmation back to server...");
                        webSocket.sendTXT("Wake-on-LAN packet sent");
                        Serial.println("[WSc] ===== WAKE PC COMMAND COMPLETED =====");
                        break;
                    
                    case CMD_PING:
                        Serial.println("[WSc] Received ping command");
                        webSocket.sendTXT("pong");
                        break;
                    
                    case CMD_UNKNOWN:
                    default:
                        Serial.printf("[WSc] Unknown command \n");
                        break;
                }
                break;
            }
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", length);
            break;
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void setup() {
    Serial.begin(115200);

    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    wifiMulti.addAP(WIFI_NAME, WIFI_PASSWORD);

    // WiFi.disconnect();
    while(wifiMulti.run() != WL_CONNECTED) {
        Serial.println("Connecting to Wi-Fi...");
        delay(100);
    }
    Serial.println("Connected successfully to Wi-Fi !");

    // Configure WebSocket connection (SSL or non-SSL)
    if (USE_SSL) {
        Serial.println("[SSL] Configuring SSL/TLS WebSocket connection...");
        
        if (SKIP_SSL_VERIFICATION) {
            Serial.println("[SSL] WARNING: Skipping SSL certificate verification (development mode)");
            // Configure SSL client to skip verification
            client.setInsecure();
        } else if (USE_CA_BUNDLE) {
            Serial.println("[SSL] Using CA certificate bundle");
            // Use built-in CA bundle if available
            // client.setCACertBundle(rootca_crt_bundle_start);
        }
        
        // Begin SSL WebSocket connection
        webSocket.beginSSL(API_URL, API_PORT, API_URI, "", "wss");
        Serial.printf("[SSL] Connecting to WSS://%s:%d%s\n", API_URL, API_PORT, API_URI);
    } else {
        Serial.println("[SSL] Using insecure WebSocket connection (development mode)");
        webSocket.begin(API_URL, API_PORT_INSECURE, API_URI, "ws");
        Serial.printf("[SSL] Connecting to WS://%s:%d%s\n", API_URL, API_PORT_INSECURE, API_URI);
    }

    // Event handler
    webSocket.onEvent(webSocketEvent);

    // Set authorization token
    webSocket.setAuthorization(AUTH_TOKEN);
    
    // Optional: Add custom headers for additional security
    // webSocket.setExtraHeaders("X-Device-ID: ESP32-WOL-Device\r\n");

    // Reconnection settings
    webSocket.setReconnectInterval(5000);
    
    // SSL-specific settings
    if (USE_SSL) {
        // Enable heartbeat to keep connection alive
        webSocket.enableHeartbeat(15000, 3000, 2);
        Serial.println("[SSL] SSL WebSocket configuration completed");
    } else {
        Serial.println("[SSL] Insecure WebSocket configuration completed");
    }
}

void loop() {
    webSocket.loop();
}
