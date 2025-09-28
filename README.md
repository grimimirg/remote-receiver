ESP32 LoRa Receiver
===================

This sketch receives LoRa packets at `868 MHz`, parses a JSON payload, and prints the extracted values over the serial console. The ISR (`onReceive`) only flags the packet; parsing and printing occur in `loop()` for stability on ESP32.

Features
--------

*   Continuous LoRa RX with `LoRa.onReceive(...)` + `LoRa.receive()`.
*   Robust JSON parsing using **ArduinoJson**.
*   Clean separation between ISR (flag only) and main loop (read & parse).

Dependencies
------------

*   Arduino IDE (or PlatformIO) with ESP32 support.
*   **LoRa** library by Sandeep Mistry.
*   **ArduinoJson** library.

Pinout & Wiring
---------------

Function

ESP32 Pin

LoRa Module

MOSI

GPIO 23

MOSI

MISO

GPIO 19

MISO

SCK

GPIO 18

SCK

SS/CS (defined)

GPIO 5

NSS

RESET (defined)

GPIO 14

RESET

DIO0 (defined)

GPIO 2

DIO0

Power

3.3V / GND

VCC (3.3V) / GND

**Note:** Keep LoRa module at 3.3 V and attach a proper 868 MHz antenna.

Configuration
-------------

*   `LORA_FREQ`: `868E6` (must match the transmitter).
*   `SYNC_WORD`: `0xF0` (must match the transmitter).

How It Works
------------

1.  `LoRa.onReceive(...)` ISR sets a flag when a packet arrives.
2.  In `loop()`, when flagged, read all available bytes from LoRa FIFO into a `String`.
3.  Parse JSON with `ArduinoJson`; extract `left` and `right` floats.
4.  Print: `Left = %.2f || Right = %.2f`, then call `LoRa.receive()` to continue RX.

Expected Packet Format
----------------------

    {
      "to": "receiver",
      "left":  0.00 .. 100.00,
      "right": 0.00 .. 100.00
    }
    

Serial Example
--------------

    LoRa ready!
    Left = 37.15 || Right = 62.80
    

Troubleshooting
---------------

*   If nothing prints, verify frequency/sync word match and wiring for SS/RST/DIO0.
*   If you see resets (Guru Meditation), ensure the ISR only sets a flag; do not parse or print inside the ISR.

License
-------

Add your preferred license (e.g., MIT).
