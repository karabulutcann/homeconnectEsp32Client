#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>

// Let's Encrypt Root Certificate (used by Koyeb)
// This is the ISRG Root X1 certificate that Let's Encrypt uses
const char *root_ca =
  "-----BEGIN CERTIFICATE-----\n"
  "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\n"
  "-----END CERTIFICATE-----\n";

// SSL Configuration for Koyeb (Let's Encrypt certificates)
const bool SKIP_SSL_VERIFICATION = false;  // Koyeb uses valid Let's Encrypt certificates
const bool USE_CA_BUNDLE = true;           // Use CA bundle for Let's Encrypt verification

WiFiMulti wifiMulti;
WebSocketsClient webSocket;
WiFiClientSecure client;

const char *WIFI_PASSWORD = "sXhbEFWn4XNj";
const char *WIFI_NAME = "FiberHGW_HUQV5N";
const char *AUTH_TOKEN = "Bearer e61c502a99f92b0d222243bbd4146a904facf04a8f79f54555cf5ff452e51856";
const char *API_URL = "hot-miranda-sentetic-8e81e0ba.koyeb.app";  // Replace with your Koyeb app URL
const int API_PORT = 443;                                         // Koyeb uses standard HTTPS port
const int API_PORT_INSECURE = 3001;                               // Local development fallback
const char *API_URI = "/socket";

// SSL Configuration for Koyeb
bool USE_SSL = true;  // Enable SSL for Koyeb deployment

// Wake-on-LAN configuration
const char *PC_MAC_ADDRESS = "2c:f0:5d:6d:44:26";  // Replace with your PC's MAC address
const char *BROADCAST_IP = "192.168.1.255";        // Replace with your network's broadcast IP
const int WOL_PORT = 9;                            // Standard WOL port

WiFiUDP udp;

// Connection monitoring variables
unsigned long lastHeartbeat = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastConnectionAttempt = 0;
bool isWebSocketConnected = false;
const unsigned long WIFI_CHECK_INTERVAL = 10000;  // Check WiFi every 10 seconds
const unsigned long HEARTBEAT_INTERVAL = 30000;   // Send heartbeat every 30 seconds
const unsigned long CONNECTION_TIMEOUT = 60000;   // Consider connection dead after 60 seconds

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
             &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
      != 6) {
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

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected from server!\n");
      Serial.printf("[WSc] WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      Serial.printf("[WSc] Free Heap: %d bytes\n", ESP.getFreeHeap());
      isWebSocketConnected = false;
      digitalWrite(LED_BUILTIN, LOW);  // Turn off LED when disconnected
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      digitalWrite(LED_BUILTIN, HIGH);  // Turn on LED when connected
      isWebSocketConnected = true;
      lastHeartbeat = millis();
      
      // send message to server when Connected
      webSocket.sendTXT("Connected");
      Serial.println("[WSc] Connection established and ready for commands");
      break;
    case WStype_TEXT:
      {
        Serial.printf("[WSc] get text: %s\n", payload);

        String cmdStr = String((char *)payload);
        Command cmd = getCommand(cmdStr);

        switch (cmd) {
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
      Serial.printf("[WSc] ERROR: WebSocket error occurred!\n");
      Serial.printf("[WSc] Error payload: %s\n", payload ? (char*)payload : "NULL");
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case WStype_FRAGMENT_TEXT_START:
      Serial.printf("[WSc] Fragment text start\n");
      break;
    case WStype_FRAGMENT_BIN_START:
      Serial.printf("[WSc] Fragment binary start\n");
      break;
    case WStype_FRAGMENT:
      Serial.printf("[WSc] Fragment received\n");
      break;
    case WStype_FRAGMENT_FIN:
      Serial.printf("[WSc] Fragment finished\n");
      break;
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  pinMode(LED_BUILTIN, OUTPUT);     // Dahili mavi led cikis olarak ayarlandi
  

  wifiMulti.addAP(WIFI_NAME, WIFI_PASSWORD);

  // WiFi.disconnect();
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Connecting to Wi-Fi...");
    digitalWrite(LED_BUILTIN, HIGH);  // Ledin ilk durumu ayarlandi
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);  // Ledin ilk durumu ayarlandi
  }

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Connected successfully to Wi-Fi !");

  // Configure WebSocket connection (SSL or non-SSL)
  if (USE_SSL) {
    Serial.println("[SSL] Configuring SSL/TLS WebSocket connection...");

    if (SKIP_SSL_VERIFICATION) {
      Serial.println("[SSL] WARNING: Skipping SSL certificate verification (development mode)");
      // Configure SSL client to skip verification
      client.setInsecure();
    } else {
      Serial.println("[SSL] Using Let's Encrypt root certificate for Koyeb");
      // Set the Let's Encrypt root certificate for Koyeb SSL verification
      client.setCACert(root_ca);
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
  unsigned long currentTime = millis();
  
  // Main WebSocket loop
  webSocket.loop();
  
  // Check WiFi connection periodically
  if (currentTime - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = currentTime;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] WiFi connection lost! Attempting to reconnect...");
      digitalWrite(LED_BUILTIN, LOW);
      
      // Try to reconnect WiFi
      while (wifiMulti.run() != WL_CONNECTED) {
        Serial.println("[WiFi] Reconnecting to Wi-Fi...");
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(400);
      }
      
      Serial.println("[WiFi] WiFi reconnected successfully!");
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
  
}
