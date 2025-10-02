#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

/*
  Remote Receiver (ESP32 + SX127x, 433 MHz) — ESP32 core v3.x API only

  - Listens for 8-byte LoRa control packets and drives two PWM outputs.
  - New LEDC API (v3.x): ledcAttach(pin, freq, res) + ledcWrite(pin, duty).
  - Fail-safe: if no valid packet arrives for 200 ms, outputs are set to 0.

  Packet layout (8 bytes total, little-endian for fields >1 byte):
    [0] 0xC3, [1] 0xA5, [2] 0x01, [3] seqLo, [4] seqHi, [5] L, [6] R, [7] CRC8

  Notes:
  - Uses Sandeep Mistry "LoRa" library.
  - PWM resolution is 8-bit (0..255) to match transmitted channel values.
*/

// ==== Pin configuration ====
#define LORA_FREQ_HZ 433E6
#define LORA_SCK     18
#define LORA_MISO    19
#define LORA_MOSI    23
#define LORA_CS       5
#define LORA_RST     14
#define LORA_DIO0    26

#define PIN_OUT_LEFT    25
#define PIN_OUT_RIGHT   27
#define PWM_FREQ        5000      // 5 kHz PWM is a safe, inaudible choice
#define PWM_RES_BITS    8         // 8-bit (0..255) duty range

// If no valid packet arrives within this window, outputs go to 0.
static const uint32_t FAILSAFE_TIMEOUT_MS = 200;

// Tracks time of the last valid packet for fail-safe logic.
static uint32_t lastValidPacketMillis = 0;

// ==== CRC-8 Dallas/Maxim (poly 0x31, reflected, init 0x00) ====
// Must match the transmitter exactly; lightweight integrity check.
static uint8_t crc8Dallas(const uint8_t* data, size_t length) {
  uint8_t crc = 0x00;
  for (size_t byteIndex = 0; byteIndex < length; ++byteIndex) {
    uint8_t currentByte = data[byteIndex];
    for (uint8_t bitIndex = 0; bitIndex < 8; ++bitIndex) {
      uint8_t mix = (crc ^ currentByte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;    // 0x8C is 0x31 reflected
      currentByte >>= 1;
    }
  }
  return crc;
}

// Convenience: write both PWM outputs (new v3.x API uses pin numbers).
static inline void setOutputs(uint8_t leftValue, uint8_t rightValue) {
  ledcWrite(PIN_OUT_LEFT,  leftValue);
  ledcWrite(PIN_OUT_RIGHT, rightValue);
}

void setup() {
  Serial.begin(115200);

  // --- LEDC (new API in ESP32 core v3.x) ---
  // Attach each pin with desired frequency and resolution once at boot.
  ledcAttach(PIN_OUT_LEFT,  PWM_FREQ, PWM_RES_BITS);
  ledcAttach(PIN_OUT_RIGHT, PWM_FREQ, PWM_RES_BITS);
  setOutputs(0, 0);  // start safe

  // --- LoRa radio init ---
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin((long)LORA_FREQ_HZ)) {
    Serial.println("LoRa initialization failed!");
    while (true);
  }

  Serial.println("LoRa initialization success!");

  //=============================
  // NOTE: Keep these settings aligned with the transmitter
  //=============================
  LoRa.setSpreadingFactor(7);         // lower latency, reasonable range
  LoRa.setSignalBandwidth(125E3);     // 125 kHz is a common default
  LoRa.setCodingRate4(5);             // 4/5: moderate robustness
  LoRa.enableCrc();                   // PHY CRC in addition to our payload CRC
  LoRa.setSyncWord(0x12);             // change to isolate from other nodes
  //=============================
}

void loop() {
  // parsePacket() > 0 means a packet is available; value is its length in bytes
  const int packetLength = LoRa.parsePacket();
  const uint32_t nowMillis = millis();

  Serial.printf("RSSI: %d dBm | SNR: %.1f dB - ", LoRa.packetRssi(), LoRa.packetSnr());

  // Accept only the expected fixed-size packet (8 bytes)
  if (packetLength == 8) {
    uint8_t packet[8];
    for (int index = 0; index < 8; ++index) {
      packet[index] = LoRa.read();
    }

    // Validate magic, version, and CRC before using data
    const bool hasValidMagic    = (packet[0] == 0xC3 && packet[1] == 0xA5);
    const bool hasValidVersion  = (packet[2] == 1);
    const bool hasValidCrc      = (crc8Dallas(packet, 7) == packet[7]);

    const bool isPacketValid    = hasValidMagic && hasValidVersion && hasValidCrc;

    Serial.printf("Packet valid -> %s\n", isPacketValid ? "true" : "false");

    if (isPacketValid) {
      // Sequence [3..4] available if you want ordering/stats
      const uint8_t leftOutputValue  = packet[5];  // 0..255
      const uint8_t rightOutputValue = packet[6];  // 0..255

      Serial.printf("Left -> %u | Right -> %u\n", (unsigned)leftOutputValue, (unsigned)rightOutputValue);

      setOutputs(leftOutputValue, rightOutputValue);

      // refresh fail-safe timer
      lastValidPacketMillis = nowMillis;           
    } else {
      Serial.println("Invalid packet received!");
    }
  } else {
    Serial.printf("Packet length: %d\n", packetLength);
  }

  // Fail-safe: zero the outputs if packets stop arriving
  if (nowMillis - lastValidPacketMillis > FAILSAFE_TIMEOUT_MS) {
    Serial.println("Fail-safe");
    setOutputs(0, 0);
  }
}
