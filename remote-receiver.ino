#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// --- LoRa ---
#define LORA_SS     5
#define LORA_RST    14
#define LORA_DIO0   2

#define LORA_FREQ   868E6
#define SYNC_WORD   0xF0

volatile bool packetReceived = false;

//------------------------------------------------------------------------------------------------

void setupLoRa() {
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed");
    while (true) { }
  }

  LoRa.setSyncWord(SYNC_WORD);
  Serial.println("LoRa ready!");
  delay(2000);
}

void setLoRaReceiveMode() {
  LoRa.onReceive(onSensorPacketReceived);
  LoRa.receive();
}

void onSensorPacketReceived(int packetSize) {
  if (packetSize == 0) return;
  packetReceived = true;
}

void readPayload(const String& payload) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("Parse error!");
    return;
  }

  float left  = doc["left"];
  float right = doc["right"];

  Serial.printf("Left = %.2f || Right = %.2f\n", left, right);
}

//------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  setupLoRa();
  setLoRaReceiveMode();
}

void loop() {
  if (packetReceived) {
    String payload;
    while (LoRa.available()) {
      payload += (char) LoRa.read();
    }
    
    readPayload(payload);
    packetReceived = false;
    LoRa.receive();
  }
}
