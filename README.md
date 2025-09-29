# Remote Receiver (ESP32 + LoRa)

This firmware turns an ESP32 into a two-channel LoRa receiver. It listens for 8-byte packets sent by the controller and drives two PWM outputs accordingly. A simple fail-safe forces both outputs to zero if packets stop arriving.

## Key Points

*   Fixed 8-byte packet with header and CRC-8 integrity check.
*   Fail-safe: outputs go to 0 after 200 ms without valid packets.
*   Uses the **new LEDC API (ESP32 core v3.x)**: `ledcAttach(pin, freq, res)` + `ledcWrite(pin, duty)`.
*   8-bit PWM range (0..255) matches the controller channel values.

## Hardware

*   ESP32 DevKit (any common module).
*   LoRa transceiver SX1276/SX1278 (433 MHz module).
*   **Power stage for each motor** (MOSFET + flyback diode, or a proper driver/ESC). Do not drive motors directly from the ESP32 pin.
*   Common GND between ESP32, LoRa module, and the motor driver stage.

## Pinout (default)

| Signal | ESP32 Pin |
| --- | --- |
| LoRa SCK | GPIO 18 |
| LoRa MISO | GPIO 19 |
| LoRa MOSI | GPIO 23 |
| LoRa CS (NSS) | GPIO 5 |
| LoRa RST | GPIO 14 |
| LoRa DIO0 | GPIO 26 |
| PWM OUT Left | GPIO 25 |
| PWM OUT Right | GPIO 26 |

For DC brushed motors: typical low-side N-MOSFET, 100 Ω gate resistor, 100 kΩ gate-to-GND pulldown, and a Schottky/ultrafast flyback diode across the motor.

## Packet Format (8 bytes)

```
Byte 0: 0xC3   // magic low   (overall 0xA5C3, little-endian)
Byte 1: 0xA5   // magic high
Byte 2: 0x01   // protocol version
Byte 3: seqLo  // sequence number low
Byte 4: seqHi  // sequence number high
Byte 5: L      // left  output (0..255)
Byte 6: R      // right output (0..255)
Byte 7: CRC8   // Dallas/Maxim over bytes 0..6
```

## Quick Start (Arduino IDE)

1.  Install the ESP32 core (Boards Manager) and ensure it is **v3.x** or newer.
2.  Install the library **LoRa** by Sandeep Mistry (Library Manager).
3.  Open `remote-receiver.ino`.
4.  Select your ESP32 board and the correct serial port.
5.  Upload and open Serial Monitor at 115200 baud (optional for debug).

## Configuration

Edit these constants at the top of the sketch to match your wiring and region:

```
#define LORA_FREQ_HZ 433E6    // set to your band (e.g., 433E6 or 868E6)
#define LORA_SCK     18
#define LORA_MISO    19
#define LORA_MOSI    23
#define LORA_CS       5
#define LORA_RST     14
#define LORA_DIO0    26

#define PIN_OUT_LEFT    25
#define PIN_OUT_RIGHT   26
#define PWM_FREQ        5000    // 5 kHz default; for inaudible DC motors use 20000
#define PWM_RES_BITS    8       // duty range 0..255

static const uint32_t FAILSAFE_TIMEOUT_MS = 200;  // outputs -> 0 after this window
```

Radio PHY defaults: `SF7`, `BW=125 kHz`, `CR=4/5`, `syncWord=0x12`. Must match the controller.

## How It Works

1.  The receiver waits for LoRa packets and only accepts 8-byte frames.
2.  It validates magic, version, and CRC-8; invalid packets are ignored.
3.  On a valid packet, it writes the two 8-bit values to PWM via `ledcWrite(pin, duty)`.
4.  If no valid packet arrives for `FAILSAFE_TIMEOUT_MS`, both outputs are forced to zero.

## Troubleshooting

*   **No PWM output:** ensure you are on ESP32 core v3.x and called `ledcAttach(pin, freq, res)` before `ledcWrite`.
*   **No radio reception:** verify SPI pins, `LoRa.setPins()`, frequency, and `syncWord` match the controller.
*   **Motor noise:** increase `PWM_FREQ` to ~20 kHz; add proper decoupling and flyback diode.
*   **Brownouts / resets:** separate motor supply from 3V3 rail; tie grounds together.

## Regulatory Notes

LoRa frequency, power, and duty-cycle must comply with local regulations (433 MHz/868 MHz bands vary by region).
